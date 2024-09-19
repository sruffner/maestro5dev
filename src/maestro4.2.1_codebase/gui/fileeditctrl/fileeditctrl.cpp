/****************************************************************************
FileEditCtrl.cpp : implementation file for the CFileEditCtrl control class
written by PJ Arends
pja@telus.net

For updates check http://www3.telus.net/pja/CFileEditCtrl.htm

-----------------------------------------------------------------------------
This code is provided as is, with no warranty as to it's suitability or usefulness
in any application in which it may be used. This code has not been tested for
UNICODE builds, nor has it been tested on a network ( with UNC paths ).

This code may be used in any way you desire. This file may be redistributed by any
means as long as it is not sold for profit, and providing that this notice and the
authors name are included.

If any bugs are found and fixed, a note to the author explaining the problem and
fix would be nice.
-----------------------------------------------------------------------------

created : October 2000
      
Revision History:

Some changes by :  Philippe Lhoste - PhiLho@gmx.net - http://jove.prohosting.com/~philho/
                   Perry Rapp      - PRapp@smartronix.com

November 11, 2000  - allowed the control to work with dialog templates
November 22, 2000  - register the control's window class, can now be added to dialog as custom control
January 4, 2001    - near total rewrite of the control, now derived from CEdit
                   - control can now be added to dialog template using an edit control
                   - browse button now drawn in nonclient area of control
January 5, 2001    - removed OnKillFocus(), replaced with OnDestroy()
January 15, 2001   - added DDX_ and DDV_ support
                   - modified GetStartPosition() and GetNextPathName()
                   - modified how FECOpenFile() updates the control text when multiple files are selected
                   - added FillBuffers()
                   - added support for relative paths
                   - added OnChange handler
                   - added drag and drop support
January 26, 2001   - fixed bug where SHBrowseForFolder does not like trailing slash
January 27, 2001   - fixed bug where if control is initialized with text, FillBuffers was not called.
January 28, 2001   - removed GetFindFolder() and SetFindFolder() replaced with GetFlags() and SetFlags()
                   - modified the DDX_ and DDV_ functions to accept these flags
                   - modified the Create() function to accept these flags
                   - allowed option for returned folder to contain trailing slash
                   - allowed browse button to be on the left side of the control
                   - added ScreenPointInButtonRect() to better tell if mouse cursor is over the button
                   - modified how OnDropFiles() updates the control text when multiple files are dropped
February 25, 2001  - fixed EN_CHANGE notification bug. Now parent window recieves this notification message
                     used ON_CONTROL_REFLECT_EX macro instead of ON_CONTROL_REFLECT
April 12, 2001     - added OnSize handler, fixed button drawing problem when control size changed
April 21, 2001     - added a tooltip for the browse button
May 12, 2001       - removed OnDestroy, replaced with PostNCDestroy
                   - added tooltip support to client area
                   - modified the FECBrowseForFolder and FECFolderProc functions
                   - added a one pixel neutral area between the client area and browse button when the
                     button is on the right hand side of the control. (looks better IMO)
May 29, 2001 - PL -- removed the filename from the m_pCFileDialog->m_ofn.lpstrInitialDir
                     variable, so when browsing back for file, we open the correct folder.
                   - used smaller (exact size) arrays for file, extension and path components.
                   - some cosmetic changes.
May 29, 2001       - FECFolderProc now checks for UNC path. SHBrowseForFolder can not be initialized with UNC
June 2, 2001       - modified ButtonClicked function. Now sends a WM_NOTIFY message to parent window before
                     showing dialog, allows parent window to cancel action by setting result to nonzero. also
                     sends WM_NOTIFY message to parent window after dialog closes with successful return
June 9, 2001       - added OnNcLButtonDblClk handler. Double click on button treated as two single clicks
June 23, 2001      - placed a declaration for the FECFolderProc global callback function into the header file
                   - fixed bug that occured when removing the filename from the m_pCFileDialog->m_ofn.lpstrInitialDir
                     variable when there was no file to remove
August 2, 2001     - replaced SetWindowText() with OnSetText() message handler. now correctly handles WM_SETTEXT messages
August 12, 2001    - added GetValidFolder() function and modified FECOpenFile() function. we now start browsing in the
                     correct folder -- it finally works!!!  {:o)
                   - modified SetFlags() so the button could be moved by setting the FEC_BUTTONLEFT flag
                   - removed the m_bCreatingControl variable
                   - removed the call to SetWindowPos() from the Create() and DDX_FileEditCtrl() functions. Now done in
                     SetFlags() function
August 14, 2001    - modified FECOpenFile(). Now sets the file name in CFileDialog to first file name in FileEditCtrl
August 18, 2001    - Set the tooltip font to the same font used in the CFileEditCtrl
September 2, 2001  - added the ModifyFlags() function and changed how the flags are handled
                   - modified the GetFlags() function
                   - added the FEC_MULTIPLE and FEC_MULTIPLEFILES flags
                   - added support for wildcards ( '*' and '?') in filenames
                     Involved : modifying the GetStartPosition(), GetNextPathName(), SetFlags(), and FillBuffers() functions
                                adding the ExpandWildCards() function
                                replacing the m_lpstrFiles variable with the m_Files array
                                adding the FEC_WILDCARDS flag.
September 3, 2001  - added ability to dereference shortcut files (*.lnk) 
                   - added the FEC_NODEREFERENCELINKS flag.
                   - added the DereferenceLink() function.
September 5, 2001  - fixed the Create() function - now destroys the control if the SetFlags() function fails
September 8, 2001  - added the AddFiles() function to be better able to handle shortcut (*.lnk) files
                     modified the OnDropFiles() function to be better able to handle shortcut (*.lnk) files
September 12, 2001 - PR -- added #include <shlobj.h> to the FileEditCtrl.h header file
                   - UNICODE fixes, added _T() macro in Create() function and in TRACE() calls.
                   - Perry states that the code now works perfectly with UNICODE builds and UNC paths.
                           {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)  {:o)
September 18, 2001 - added ability to use icons or bitmaps on the browse button
                     Involved : adding SetButtonImage() function.
                                modifying the DrawButton() function
                                adding the FECButtonImage class.
September 20, 2001 - fixed resource leak in FECButtonImage class
                   - cleaned up the FECButtonImage class code
                   - added ability to resize the browse button
                     Involved : adding m_nButtonWidth variable
                                adding SetButtonWidth() and GetButtonWidth() functions
                                modifying OnNcCalcSize() and DrawButton() functions
September 24, 2001 - fixed bug in GetNextPathName() and ExpandWildCards() where multiple files could not
                     start from the current drive   ie \folder\file.ext as second file would give an error
September 26, 2001 - fixed bug in GetNextPathName() to allow incomplete relative paths (ie ..\..\) when
                     browsing for files.
October 5, 2001 - PR -- Added #include <afxcmn.h> to the CFileEditCtrl.h header file
October 14, 2001   - rewrote the FECButtonimage::DrawImage() function, it now handles disabled transparent
                     bitmaps better (the transparent colour can be any colour, no long just light colours),
                     and now also handles pattern and bitmap background brushes
                   - various other touch ups (comments mostly)


MODIFICATIONS FOR USE WITH CNTRLX
---------------------------------
19feb2001.  saruffner.
Adapted CFileEditCtrl source files for use with CNTRLX.  Building as a static MFC library.  Will not use a string table 
resource.  File stdafx.h modified to handle building with Visual C++ versions prior to 6.0.  Any changes I introduce 
can be examined by searching on the tag "saruffner".

08oct2002.  saruffner.
Made SetFlags() virtual so that we can override it in a derived class.

07feb2013.  saruffner.
HACK to workaround a Vista/Win7-specific issue with CFileDialog. Disable use of Vista-style dialog.

31aug2017(sar) -- Replacing insecure CRT string manipulation methods with secure versions. Other changes IAW
warnings and errors generated while compiling for 64-bit target in VStudio 2017.
****************************************************************************/

#include "stdafx.h"
#include "fileeditctrl.h"

#ifndef __ATLCONV_H__
#include <atlconv.h>   // for T2COLE() macro in DereferenceLinks() function
#endif // __ATLCONV_H__

// resource.h is only needed for the string table resources. If you do not put the FEC_IDS_*
// strings in a string table, you do not need to include resource.h
// 19feb2001(saruffner) -- We comment out this include.  No string table in build.
// #include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// These strings should be entered into a string table resource with the 
// FEC_IDS_* identifiers defined here, although this class will work
// properly if they are not.
//
// string usage:
//
// FEC_IDS_ALLFILES         - default file filter for CFileDialog
// FEC_IDS_BUTTONTIP        - Text for the browse button tooltip
// FEC_IDS_FILEDIALOGTITLE  - default dialog caption for CFileDialog
// FEC_IDS_SEPARATOR        - character used to seperate files when OFN_ALLOWMULTISELECT flag is used
// FEC_IDS_NOFILE           - Error message for DDV_FileEditCtrl() when no files or folders are entered
// FEC_IDS_NOTEXIST         - Error message for DDV_FileEditCtrl() when the specified file or folder could not be found
// FEC_IDS_NOTFILE          - Error message for DDV_FileEditCtrl() when the specified file is actually a folder
// FEC_IDS_NOTFOLDER        - Error message for DDV_FileEditCtrl() when the specified folder is actually a file
// FEC_IDS_OKBUTTON         - Text for the 'OK' (Open) button on CFileDialog
//

// FEC_IDS_ALLFILES will be defined in resource.h if these strings
// are in a string table resource
#if !defined FEC_IDS_ALLFILES
#define FEC_NORESOURCESTRINGS /* so this class knows how to handle these strings */
#define FEC_IDS_ALLFILES        _T("All Files (*.*)|*.*||")
#define FEC_IDS_BUTTONTIP       _T("Browse")
#define FEC_IDS_FILEDIALOGTITLE _T("Browse for File")
#define FEC_IDS_SEPARATOR       _T(";")
#define FEC_IDS_NOFILE          _T("Enter an existing file.")
#define FEC_IDS_NOTEXIST        _T("%s does not exist.")
#define FEC_IDS_NOTFILE         _T("%s is not a file.")
#define FEC_IDS_NOTFOLDER       _T("%s is not a folder.")
#define FEC_IDS_OKBUTTON        _T("OK")
#endif

/////////////////////////////////////////////////////////////////////////////
// Button states
#define BTN_UP          0
#define BTN_DOWN        1
#define BTN_DISABLED    2

/////////////////////////////////////////////////////////////////////////////
// ToolTip IDs
#define ID_BUTTONTIP    1
#define ID_CLIENTTIP    2

/////////////////////////////////////////////////////////////////////////////
// Helper functions

/////////////////////////////////////////////////////////////////////////////
//
//  IsWindow  (Global function)
//    Checks if the given window is active
//
//  Parameters :
//    pWnd [in] - points to the CWnd object to check
//
//  Returns :
//    TRUE if the window is active
//    FALSE if not
//
/////////////////////////////////////////////////////////////////////////////

BOOL IsWindow(CWnd *pWnd)
{
    if (!pWnd)
        return FALSE;
    return ::IsWindow(pWnd->m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////
// CFileEditCtrl

IMPLEMENT_DYNAMIC(CFileEditCtrl, CEdit)

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl constructor  (public member function)
//    Initializes all the internal variables
//
//  Parameters :
//    bAutoDelete [in] - Auto delete flag
//
//  Returns :
//    Nothing
//
//  Note :
//    If bAutoDelete is TRUE, this class object will be deleted
//    when it's window is destroyed (in CFileEditCtrl::PostNCDestroy).
//    The only time this should be used is when the control is
//    created dynamicly in the
//    DDX_FileEditCtrl(CDataExchange*,int,CString&,DWORD) function.
//
/////////////////////////////////////////////////////////////////////////////

CFileEditCtrl::CFileEditCtrl(BOOL bAutoDelete /* = FALSE */)
{
    m_bAutoDelete       = bAutoDelete;
    m_bButtonLeft       = (DWORD)~0;  // 0xFFFFFFFF
    m_bMouseCaptured    = FALSE;
    m_bTextChanged      = TRUE;
    m_dwFlags           = 0;
    m_Files.RemoveAll();
    m_nButtonState      = BTN_UP;
    m_nButtonWidth      = -1;
    m_pButtonImage      = NULL;
    m_pBROWSEINFO       = NULL;
    m_pCFileDialog      = NULL;
    m_rcButtonRect.SetRectEmpty();
    m_szCaption.Empty();
    m_szClientTip.Empty();
    m_szFolder.Empty();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl destructor  (public member function)
//    cleans up internal data variables
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

CFileEditCtrl::~CFileEditCtrl()
{
    m_Files.RemoveAll();
    if (m_pBROWSEINFO)
        delete m_pBROWSEINFO;
    if (m_pCFileDialog)
        delete m_pCFileDialog;
}

/////////////////////////////////////////////////////////////////////////////
// Message Map
BEGIN_MESSAGE_MAP(CFileEditCtrl, CEdit)
//{{AFX_MSG_MAP(CFileEditCtrl)
ON_WM_DROPFILES()
ON_WM_ENABLE()
ON_WM_KEYDOWN()
ON_WM_LBUTTONUP()
ON_WM_MOUSEMOVE()
ON_WM_NCCALCSIZE()
ON_WM_NCHITTEST()
ON_WM_NCLBUTTONDBLCLK()
ON_WM_NCLBUTTONDOWN()
ON_WM_NCPAINT()
ON_WM_SETFOCUS()
ON_WM_SIZE()
//}}AFX_MSG_MAP
ON_CONTROL_REFLECT_EX(EN_CHANGE, OnChange)
ON_MESSAGE(WM_SETTEXT, OnSetText)
ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnTTNNeedText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileEditCtrl member functions

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::AddFile  (protected member function)
//    Adds the specified file to the m_Files array. Removes the path info
//    if it is the same as the path in m_szFolder
//
//  Parameters :
//    FileName [in] - The file to add
//
//  Returns :
//    Nothing
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Delete()
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::AddFile(CString File)
{
   if( !(GetFlags() & FEC_NODEREFERENCELINKS) )
   {
      CString Ext = File.Right( 4 );
      Ext.MakeLower();
      if( Ext == _T(".lnk") )
         DereferenceLink( File );
   }
   int FolderLength = m_szFolder.GetLength();

#ifdef _USING_VC60
   if( File.Left( FolderLength ) == m_szFolder )
      File.Delete( 0, FolderLength );
   m_Files.Add( File );
#else
   if( File.Left( FolderLength ) == m_szFolder )
   {
      CString FileNoPath;
      if( File.GetLength() > FolderLength )
         FileNoPath = File.Mid( FolderLength );
      m_Files.Add( FileNoPath );
   }
   else
      m_Files.Add( File );
#endif // _USING_VC60
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::ButtonClicked  (protected member function)
//    Called when the user clicks on the browse button.
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
//  Note :
//    Sends an WM_NOTIFY message to the parent window both before and after the dialogs have run.
//    The NMHDR parameter points to a FEC_NOTIFY structure.
//
//    Before : Sends the FEC_NM_PREBROWSE notification. Returning non-zero aborts this function.
//    After  : Sends the FEC_NM_POSTBROWSE notification.
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::ButtonClicked()
{
    BOOL bResult = FALSE;
    CWnd *pParent = GetParent();
    if (IsWindow(pParent))
    {
        FEC_NOTIFY notify(this, FEC_NM_PREBROWSE);
        if (pParent->SendMessage (WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&notify) != 0)
            return; // SendMessage returned nonzero, do not show dialog
    }
    DWORD Flags = GetFlags();
    if (Flags & FEC_FOLDER)
        bResult = FECBrowseForFolder();
    else if (Flags & FEC_FILE)
        bResult = FECOpenFile();
    else
    {
        ASSERT (FALSE);  // control flags not properly set (should never get here)
    }
    if (bResult && IsWindow(pParent))
    {
        FEC_NOTIFY notify(this, FEC_NM_POSTBROWSE);
        pParent->SendMessage (WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&notify);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::Create  (public member function)
//    Creates the CFileEditCtrl in any window.
//
//  Parameters :
//    dwFlags        [in] - CFileEditCtrl flags ( FEC_* )
//    dwExStyle      [in] - Windows extended styles ( WS_EX_* )
//    lpszWindowName [in] - The initial text in the control
//    dwStyle        [in] - Windows and Edit control styles ( WS_* and ES_* )
//    rect           [in] - The position and size of the control
//    pParentWnd     [in] - Pointer to the control's parent window
//    nID            [in] - the control's ID
//
//  Returns :
//    TRUE if the control was successfully created
//    FALSE if not
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::Create(DWORD dwFlags,
                           DWORD dwExStyle,
                           LPCTSTR lpszWindowName,
                           DWORD dwStyle,
                           const RECT& rect,
                           CWnd* pParentWnd,
                           UINT nID) 
{
    BOOL bResult = CreateEx(dwExStyle,
                            _T("EDIT"),
                            lpszWindowName,
                            dwStyle,
                            rect,
                            pParentWnd,
                            nID);
    if (bResult)
    {   // call CFileEditCtrl::SetFlags() to initialize the internal data structures
        bResult = SetFlags(dwFlags);
        if (bResult)
        {   // set the font to the font used by the parent window
            if (IsWindow(pParentWnd))
                SetFont(pParentWnd->GetFont());
        }
        else   // SetFlags() failed - destroy the window
            DestroyWindow();
    }
    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::DereferenceLinks  (protected member function)
//    Gets the file path name pointed to by shortcut (*.lnk) file
//
//  Parameters :
//    FileName [in]  : the shortcut file to be dereferenced
//             [out] : if successful, the complete path name of the file the
//                     shortcut points to, or unchanged if not successful
//
//  Returns :
//    TRUE if the link was dereferenced
//    FALSE if not
//
//  Note :
//    Thanks to Michael Dunn for his article "Introduction to COM - What It Is and How to Use It."
//    found at http://www.codeproject.com/com/comintro.asp
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::DereferenceLink(CString &FileName)
{
    BOOL ret = FALSE;       // assume failure
    IShellLink *pIShellLink;
    IPersistFile *pIPersistFile;

    // initialize the COM libraries
    CoInitialize (NULL);

    // create an IShellLink object
    HRESULT hr = CoCreateInstance(CLSID_ShellLink,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IShellLink,
                                  (void **) &pIShellLink);
    if (SUCCEEDED (hr))
    {
        // get the IPersistFile interface for this IShellLink object
        hr = pIShellLink->QueryInterface(IID_IPersistFile, (void **)&pIPersistFile);
        if (SUCCEEDED (hr))
        {
            int len = FileName.GetLength();
            WCHAR *pWFile = new WCHAR[len + 1];

            USES_CONVERSION;
            wcscpy_s(pWFile, len+1, T2COLE(FileName));

            // open and read the .lnk file
            hr = pIPersistFile->Load(pWFile, 0);
            if (SUCCEEDED(hr))
            {
                TCHAR buffer[_MAX_PATH];
                // get the file path name 
                hr = pIShellLink->GetPath(buffer, _MAX_PATH, NULL, 0 /*SLGP_UNCPRIORITY*/);
                if (SUCCEEDED(hr))
                {
                    FileName = buffer;
                    ret = TRUE;
                }
            }
            delete[] pWFile;
        }

        // release the IShellLink interface
        pIShellLink->Release();
    }

    // close the COM libraries
    CoUninitialize();

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::DrawButton  (protected member function)
//    Draws the button on the control
//
//  Parameters :
//    nButtonState [in] - the state of the button ( Up, Down, or Disabled )
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::DrawButton(int nButtonState)
{   // if the button is too small, do not draw it
    if (m_rcButtonRect.Width() < 3 || m_rcButtonRect.Height() < 3)
        return;

    ASSERT(IsWindow(this));

    // if the control is disabled, ensure the button is drawn disabled
    if (GetStyle() & WS_DISABLED)
        nButtonState = BTN_DISABLED;

    CWindowDC DC(this);     // get the DC for drawing

    CBrush theBrush(GetSysColor(COLOR_3DFACE));     // the colour of the button background
    CBrush *pOldBrush = DC.SelectObject(&theBrush);

    if (nButtonState == BTN_DOWN)   // Draw button as down
    {
        // draw the border
        CPen thePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
        CPen *pOldPen = DC.SelectObject(&thePen);
        DC.Rectangle(&m_rcButtonRect);
        DC.SelectObject(pOldPen);
        thePen.DeleteObject();
        
        if (m_pButtonImage)
        {   // draw the image
            if (m_rcButtonRect.Width() > 4 && m_rcButtonRect.Height() > 4)
                m_pButtonImage->DrawImage(&DC,
                m_rcButtonRect.left + 3,
                m_rcButtonRect.top + 3,
                m_rcButtonRect.Width() - 4,
                m_rcButtonRect.Height() - 4);
        }
        else   // draw the dots
            DrawDots(&DC, GetSysColor(COLOR_BTNTEXT), 1);
    }
    else    // draw button as up
    {
        DC.FillRect(&m_rcButtonRect, &theBrush);
        
        // draw the border
        DC.DrawEdge(&m_rcButtonRect, EDGE_RAISED, BF_RECT);
        
        
        if (nButtonState == BTN_DISABLED)
        {
            if (m_pButtonImage)
            {   // draw the image
                if (m_rcButtonRect.Width() > 4 && m_rcButtonRect.Height() > 4)
                    m_pButtonImage->DrawImage(&DC,
                    m_rcButtonRect.left + 2,
                    m_rcButtonRect.top + 2,
                    m_rcButtonRect.Width() - 4,
                    m_rcButtonRect.Height() - 4,
                    TRUE);
            }
            else
            {   // draw the dots
                DrawDots(&DC, GetSysColor(COLOR_3DHILIGHT), 1);
                DrawDots(&DC, GetSysColor(COLOR_3DSHADOW));
            }
        }
        else if (nButtonState == BTN_UP)
        {
            if (m_pButtonImage)
            {   // draw the image
                if (m_rcButtonRect.Width() > 4 && m_rcButtonRect.Height() > 4)
                    m_pButtonImage->DrawImage(&DC,
                    m_rcButtonRect.left + 2,
                    m_rcButtonRect.top + 2,
                    m_rcButtonRect.Width() - 4,
                    m_rcButtonRect.Height() - 4);
            }
            else   // draw the dots
                DrawDots(&DC, GetSysColor(COLOR_BTNTEXT));
        }
        else
        {
            ASSERT(FALSE);  // Invalid nButtonState
        }
    }
    DC.SelectObject(pOldBrush);
    theBrush.DeleteObject();

    // update m_nButtonState
    m_nButtonState = nButtonState;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::DrawDots  (protected member function)
//    Draws the dots on the button
//
//  Parameters :
//    pDC     [in] - Pointer to the device context needed for drawing
//    CR      [in] - The colour of the dots
//    nOffset [in] - How far down and to the right to shift the dots
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::DrawDots(CDC *pDC, COLORREF CR, int nOffset /* = 0 */)
{
    int width = m_rcButtonRect.Width();         // width of the button
    div_t divt = div(width, 4);
    int delta = divt.quot;                      // space between dots
    int left = m_rcButtonRect.left + width / 2 - delta - (divt.rem ? 0 : 1); // left side of first dot
    width = width / 10;                         // width and height of one dot
    int top = m_rcButtonRect.Height() / 2 - width / 2 + 1;  // top of dots
    left += nOffset;                            // draw dots shifted? (for button pressed)
    top += nOffset;
    // draw the dots
    if (width < 2)
    {
        pDC->SetPixel(left, top, CR);
        left += delta;
        pDC->SetPixel(left, top, CR);
        left += delta;
        pDC->SetPixel(left, top, CR);
    }
    else
    {
        CPen thePen(PS_SOLID, 1, CR);           // set the dot colour
        CPen *pOldPen = pDC->SelectObject(&thePen);
        CBrush theBrush(CR);
        CBrush *pOldBrush = pDC->SelectObject(&theBrush);
        pDC->Ellipse(left, top, left + width, top + width);
        left += delta;
        pDC->Ellipse(left, top, left + width, top + width);
        left += delta;
        pDC->Ellipse(left, top, left + width, top + width);
        pDC->SelectObject(pOldBrush);           // reset the DC
        theBrush.DeleteObject();
        pDC->SelectObject(pOldPen);
        thePen.DeleteObject();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::ExpandWildCards  (protected member function)
//    resolves any wildcards ('*' and/or '?') found in the file name
//    calls AddFile() to add the files to the m_Files array
//
//  Parameters :
//    FileName [in] - the file name
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::ExpandWildCards(const CString &FileName)
{
    DWORD Flags = GetFlags();
    if (!(Flags & FEC_WILDCARDS) || FileName.FindOneOf(_T("*?")) == -1)
    {   // wildcards not permitted or not found
        AddFile(FileName);
        return;
    }
    CString Temp;
    CString Path;
    if (FileName[0] == _T('\\') || (FileName.GetLength() > 1 && FileName[1] == _T(':')))
        Temp = FileName;
    else
        Temp = m_szFolder + FileName;

    _tfullpath(Path.GetBuffer(_MAX_PATH), Temp, _MAX_PATH);
    Path.ReleaseBuffer();
    CFileFind cff;
    BOOL Finding = cff.FindFile(Path);
    while (Finding)
    {
        Finding = cff.FindNextFile();
        Path = cff.GetFilePath();
        if (!cff.IsDirectory())
        {
            AddFile(Path);
            if (!(Flags & FEC_MULTIPLE))
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::FECBrowseForFolder  (protected member function)
//    Set up and call SHBrowseForFolder().
//    Update the control to the users selection
//
//  Parameters :
//    None
//
//  Returns :
//    TRUE if user clicks OK in SHBrowseForFolder dialog
//    FALSE otherwise
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::FECBrowseForFolder()
{
    BOOL bReturnValue = FALSE;

#if defined _DEBUG
    if (m_pBROWSEINFO->lpfn == FECFolderProc)
        ASSERT((CWnd *)m_pBROWSEINFO->lParam == this);
#endif

    ITEMIDLIST *idl = static_cast<ITEMIDLIST *>(SHBrowseForFolder(m_pBROWSEINFO));
    if (idl)
    {
        TCHAR lpstrBuffer[_MAX_PATH] = _T("");
        if (SHGetPathFromIDList(idl, lpstrBuffer))  // get path string from ITEMIDLIST
        {
            if (GetFlags() & FEC_TRAILINGSLASH) // add a trailing slash if it is not already there
            {
                size_t len = _tcslen(lpstrBuffer);
                if (lpstrBuffer[len - 1] != _T('\\'))
                    _tcscat_s(lpstrBuffer, _T("\\"));
            }
            SetWindowText(lpstrBuffer);         // update edit control
            bReturnValue = TRUE;
        }
        LPMALLOC lpm;
        if (SHGetMalloc(&lpm) == NOERROR)
            lpm->Free(idl);                     // free memory returned by SHBrowseForFolder
    }
    SetFocus();                                 // ensure focus returns to this control
    return bReturnValue;
}

/////////////////////////////////////////////////////////////////////////////
//
//  FECFolderProc  (Global CALLBACK function)
//    This is the default callback procedure for the SHBrowseForFolder function.
//    It sets the initial selection to the directory specified in the edit control
//
//  Parameters :
//    hwnd   [in] - Handle of the SHBrowseForFolder dialog
//    nMsg   [in] - Message to be handled
//    lpData [in] - the lparam member of the BROWSEINFO structure, must be a pointer
//                  to the CFileEditCtrl
//
//  Returns :
//    zero
//
//  Note :
//    See 'SHBrowseForFolder' in MSDN for more info on the callback function for SHBrowseForFolder()
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Delete()
/////////////////////////////////////////////////////////////////////////////

int CALLBACK FECFolderProc(HWND hWnd, UINT nMsg, LPARAM, LPARAM lpData)
{
   if( nMsg == BFFM_INITIALIZED )                              // ensure we are passed a pointer to the CFileEditCtrl 
   {
      CWnd *pWnd = (CWnd *)lpData;
      ASSERT( pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)) );
      CFileEditCtrl* pCFEC = (CFileEditCtrl *)pWnd;

      CString szPath;
      POSITION pos = pCFEC->GetStartPosition();
      if( pos )
      {
         szPath = pCFEC->GetNextPathName(pos);
         if( szPath.Left(2) != _T("\\\\") )                    // SHBrowseForFolder does not like UNC path
         {
            int len = szPath.GetLength() - 1;
            if( len != 2 && szPath[len] == _T('\\') )          // SHBrowseForFolder does not like trailing slash 
            {
#ifdef _USING_VC60
               szPath.Delete(len); 
#else
               CString strTemp = szPath.Left(len);
               szPath = strTemp;
#endif // _USING_VC60
            }
            ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPCTSTR)szPath);
         }
      }
   }
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::FECOpenFile  (protected member function)
//    Set up the CFileDialog and call CFileDialog::DoModal().
//    Update the control to the users selection
//
//  Parameters :
//    None
//
//  Returns :
//    TRUE if the user clicked the OK button in the CFileDialog
//    FALSE otherwise
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::FECOpenFile()
{
    BOOL bReturnValue = FALSE;
    BOOL bDirectory = TRUE;                     // assume user of this class has set the initial directory
    TCHAR lpstrDirectory[_MAX_PATH] = _T("");
    if (m_pCFileDialog->m_ofn.lpstrInitialDir == NULL)  // user has not set the initial directory
    {                                                   // flag it, and set the initial directory
        bDirectory = FALSE;                             // to the directory in the edit control
        POSITION pos = GetStartPosition();
        if (pos)
        {
            _tcscpy_s(lpstrDirectory, GetNextPathName(pos));
            
            DWORD attrib = GetFileAttributes(lpstrDirectory);
            if (((attrib != 0xFFFFFFFF) && (!(attrib & FILE_ATTRIBUTE_DIRECTORY)))
                || ((attrib == 0xFFFFFFFF) && (!(m_pCFileDialog->m_ofn.Flags & OFN_FILEMUSTEXIST))))
                // if ((file exists && is not a folder) || (does not exist && does not have to exist))
            {
                // set the filename editbox in CFileDialog to the name of the
                // first file in the control
                TCHAR Name[_MAX_FNAME];
                TCHAR Ext[_MAX_EXT];
                _TCHAR drive[_MAX_DRIVE];
                _TCHAR folder[_MAX_DIR];
                _tsplitpath_s(lpstrDirectory, drive, folder, Name, Ext);
                _tcscat_s(Name, Ext);
                _tcscpy_s(m_pCFileDialog->m_ofn.lpstrFile, m_pCFileDialog->m_ofn.nMaxFile, Name);
            }
            else
                // empty the filename edit box
                _tcscpy_s(m_pCFileDialog->m_ofn.lpstrFile, m_pCFileDialog->m_ofn.nMaxFile, _T(""));
            
            // Start browsing in the correct folder
            GetValidFolder(lpstrDirectory, _MAX_PATH);
            m_pCFileDialog->m_ofn.lpstrInitialDir = lpstrDirectory;
        }
    }
    
    if (m_pCFileDialog->DoModal() == IDOK)      // Start the CFileDialog
    {                                           // user clicked OK, enter files selected into edit control
        CString szFileSeparator;
#if defined FEC_NORESOURCESTRINGS
        szFileSeparator = FEC_IDS_SEPARATOR;
#else
        szFileSeparator.LoadString(FEC_IDS_SEPARATOR);
#endif
        ASSERT(szFileSeparator.GetLength() == 1);   // must be one character only
        szFileSeparator += _T(" ");
        CString szPath;
        POSITION pos = m_pCFileDialog->GetStartPosition();
        if (pos)                                // first file has complete path
            szPath = m_pCFileDialog->GetNextPathName(pos);
        TCHAR lpstrName[_MAX_FNAME];
        TCHAR lpstrExt[_MAX_EXT];
        _TCHAR drive[_MAX_DRIVE];
        _TCHAR folder[_MAX_DIR];

        CString szTempPath;
        while (pos)
        {                                       // remaining files are name and extension only
            szTempPath = m_pCFileDialog->GetNextPathName(pos);
            _tsplitpath_s(szTempPath, drive, folder, lpstrName, lpstrExt);
            szPath += szFileSeparator + lpstrName + lpstrExt;
        }
        SetWindowText(szPath);
        bReturnValue = TRUE;
    }
    if (!bDirectory)                            // reset OPENFILENAME
        m_pCFileDialog->m_ofn.lpstrInitialDir = NULL;

    SetFocus();                                 // ensure focus returns to this control
    return bReturnValue;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::FillBuffers  (protected member function)
//    Fills the m_szFolder and m_Files member variables
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
//  Note :
//    The m_szFolder and m_Files member variables are used by the GetStartPosition()
//    and GetNextPathName() functions to retreive the file names entered by the user.
//
//    If the user entered a folder, m_Files will contain the complete path for the
//    folder, and m_szFolder will be empty
//
//    If the user entered multiple files, m_szFolder will contain the drive and folder
//    path of the first file entered, and m_Files will contain all the files. The files
//    may contain any complete or relative paths. Any relative paths will be evaluated
//    as being relative to the path contained in m_szFolder.
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Delete()
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::FillBuffers()
{
    ASSERT(IsWindow(this));                     // Control window must exist
#if defined FEC_NORESOURCESTRINGS
    m_szFolder = FEC_IDS_SEPARATOR;
#else
    m_szFolder.LoadString(FEC_IDS_SEPARATOR);
#endif
    TCHAR chSeparator = m_szFolder[0];          // get the character used to seperate the files

    m_szFolder.Empty();                         // empty the buffers of old data
    m_Files.RemoveAll();

    int len = GetWindowTextLength();
    if (!len)                                   // no files entered, leave buffers empty
        return;
    LPTSTR lpstrWindow = new TCHAR[len + 1];    // working buffer
    GetWindowText(lpstrWindow, len + 1);
    LPTSTR lpstrStart = lpstrWindow;            // points to the first character in a file name
    LPTSTR lpstrEnd = NULL;                     // points to the next separator character
    while (*lpstrStart == chSeparator || _istspace(*lpstrStart))
        lpstrStart++;                           // skip over leading spaces and separator characters
    if (!*lpstrStart)
    {                                           // no files entered, leave buffers empty
        delete lpstrWindow;
        return;
    }
    lpstrEnd = _tcschr(lpstrStart, chSeparator);// find separator character
    if (lpstrEnd)
        *lpstrEnd = 0;                          // mark end of string
    LPTSTR temp = lpstrStart + _tcslen(lpstrStart) - 1;
    while (_istspace(*temp))                    // remove trailing white spaces from string
    {
        *temp = 0;
        temp--;
    }
    DWORD dwFlags = GetFlags();
    CString File;
    if (dwFlags & FEC_FOLDER)
    {
        _tfullpath(File.GetBuffer(_MAX_PATH), lpstrStart, _MAX_PATH); // get absolute path
        File.ReleaseBuffer();
        int len = File.GetLength();
        if (dwFlags & FEC_TRAILINGSLASH)        // add a trailing slash if it is not already there
        {
            if (File[len - 1] != _T('\\'))
                File += _T("\\");
        }
        else                                    // remove the trailing slash if it is there
        {
            if (len != 3 && File[len - 1] == _T('\\'))
            {
#ifdef _USING_VC60
               File.Delete( len - 1 ); 
#else
               CString strTemp = File.Left( len - 1 );
               File = strTemp;
#endif // _USING_VC60
            }
        }
        m_Files.Add(File);
        delete lpstrWindow;
        return;
    }
    _TCHAR drive[_MAX_DRIVE];
    _TCHAR folder[_MAX_DIR];
    _TCHAR file[_MAX_FNAME];
    _TCHAR ext[_MAX_EXT];
    _tsplitpath_s(lpstrStart, drive, folder, file, ext);
    m_szFolder = (CString)drive + folder;       // drive and directory placed in m_szFolder
    File = (CString)file + ext;
    ExpandWildCards(File);
    if (dwFlags & FEC_MULTIPLE)
    {
        lpstrStart = lpstrEnd + 1;              // reset to the start of the next string
        while (lpstrEnd)
        {   // add the rest of the files as they have been typed (include any path information)
            while (*lpstrStart == chSeparator || _istspace(*lpstrStart))
                lpstrStart++;                   // remove leading spaces and separator characters
            if (!*lpstrStart)                   // last file was followed by spaces and/or separator characters,
                break;                          // there are no more files entered
            lpstrEnd = _tcschr(lpstrStart, chSeparator); // find next separator character
            if (lpstrEnd)
                *lpstrEnd = 0;                  // mark end of string
            temp = lpstrStart + _tcslen(lpstrStart) - 1;
            while (_istspace(*temp))            // remove trailing white spaces from string
            {
                *temp = 0;
                temp--;
            }
            ExpandWildCards(lpstrStart);
            if (lpstrEnd)
                lpstrStart = lpstrEnd + 1;      // reset to the start of the next string
        }
    }
    delete lpstrWindow;                         // delete working buffer
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetBrowseInfo  (public member function)
//    Retreive a pointer to the BROWSEINFO structure
//
//  Parameters :
//    None
//
//  Returns :
//    A pointer to the BROWSEINFO structure if the FEC_FOLDER flag was set
//    NULL otherwise
//
//  Note :
//    If the default SHBrowseForFolder settings do not fit your use, Use the pointer
//    returned by this function to set up the SHBrowseForFolder using your own settings
//    
/////////////////////////////////////////////////////////////////////////////

BROWSEINFO* CFileEditCtrl::GetBrowseInfo() const
{
    return m_pBROWSEINFO;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetButtonWidth  (public member function)
//    Retreives the width, in pixels, of the browse button 
//
//  Parameters :
//    None
//
//  Returns :
//    The width of the browse button
//
/////////////////////////////////////////////////////////////////////////////

int CFileEditCtrl::GetButtonWidth()
{
    return m_rcButtonRect.Width();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetFlags  (public member function)
//    Retreives the current flags
//
//  Parameters :
//    None
//
//  Returns :
//    the current flags
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//    Because some flags can be changed via GetOpenFileName(), always use this
//    function to get the current state of the flags. Do not use m_dwFlags directly.
//
/////////////////////////////////////////////////////////////////////////////

DWORD CFileEditCtrl::GetFlags()
{
    DWORD Flags = m_dwFlags;
    if (m_pCFileDialog)
    {   // coordinate the FEC_* flags with the OFN_* flags
        if (m_pCFileDialog->m_ofn.Flags & OFN_NODEREFERENCELINKS)
            Flags |= FEC_NODEREFERENCELINKS;
        else
            Flags &= ~FEC_NODEREFERENCELINKS;

        if (m_pCFileDialog->m_ofn.Flags & OFN_ALLOWMULTISELECT)
            Flags |= FEC_MULTIPLE;
        else
            Flags &= ~FEC_MULTIPLE;
    }
    return Flags;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetNextPathName  (public member function)
//    Returns the file name at the specified position in the buffer.
//
//  Parameters :
//    pos [in]  - The position of the file name to retreive
//        [out] - the position of the next file name
//
//  Returns :
//    The complete path name of the file or folder
//
//  Note :
//    The starting position is retreived using the GetStartPosition() function.
//    pos will be set to NULL when there are no more files
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Insert()
/////////////////////////////////////////////////////////////////////////////

CString CFileEditCtrl::GetNextPathName(POSITION &pos)
{
   ASSERT(pos);                                // pos must not be NULL
   INT_PTR index = (INT_PTR)pos - 1;
   CString ReturnString;
   CString Temp = m_Files.GetAt(index);
    
   if (Temp.GetLength() == 0)
      Temp = m_szFolder;
   else if ((Temp.GetLength() > 1 && Temp[1] != _T(':'))   // not drive
              && Temp[0] != _T('\\'))           // nor current drive nor UNC
   {
#ifdef _USING_VC60
      Temp.Insert( 0, m_szFolder );
#else
      CString str = m_szFolder + Temp;
      Temp = str;
#endif // _USING_VC60
   }

    _tfullpath(ReturnString.GetBuffer(_MAX_PATH), Temp, _MAX_PATH); // get absolute path from any relative paths
    ReturnString.ReleaseBuffer();
    
    DWORD Flags = GetFlags();
    
    if (Flags & FEC_FILE)
    {
        Temp = ReturnString.Right(4);
        Temp.MakeLower();
        if (Temp == _T(".lnk") && !(Flags & FEC_NODEREFERENCELINKS))
            // resolve shortcuts (*.lnk files)
            DereferenceLink(ReturnString);
    }
    
    index++;                                    // set pos to next file
    if (index > m_Files.GetUpperBound())
        index = -1;
    pos = (POSITION)(index + 1);
    return ReturnString;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetOpenFileName  (public member function)
//    Retreives a pointer to the OPENFILENAME structure
//
//  Parameters :
//    None
//
//  Returns :
//    A pointer to the OPENFILENAME structure if the FEC_FILE flag was set
//    NULL otherwise
//
//  Note :
//    If the default CFileDialog settings do not fit your use, Use the pointer
//    returned by this function to set up the CFileDialog using your own settings
//
/////////////////////////////////////////////////////////////////////////////

OPENFILENAME* CFileEditCtrl::GetOpenFileName() const
{
    if (m_pCFileDialog)
        return (&m_pCFileDialog->m_ofn);
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetStartPosition  (public member function)
//    If the control is active, calls FillBuffers() if the text has changed
//    returns the position of the first file in the buffers
//
//  Parameters :
//    None
//
//  Returns :
//    A MFC POSITION structure that points to the first file in the control
//
//  Note :
//    Use this function to get the starting position for the GetNextPathName() function
//
/////////////////////////////////////////////////////////////////////////////

POSITION CFileEditCtrl::GetStartPosition()
{
    if (IsWindow(this) && m_bTextChanged)
    {
        FillBuffers();
        m_bTextChanged = FALSE;
    }
    return (POSITION)(m_Files.GetSize() ? ((INT_PTR)1) : NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::GetValidFolder  (protected member function)
//    Removes all files and nonexistant folders from the given path
//    Adds a slash to the end of the path string if it is not already there
//
//  Parameters :
//    Path [in]  - The path to check
//         [out] - The new path
//
//  Returns :
//    TRUE if the original path is valid
//    FALSE if the original path was invalid and has been changed
//
//  Note :
//    Not tested for UNC paths {:o(
//    If this does not work, and you make changes to get it to work, drop me 
//    note at pja@telus.net
//    Thanks {:o)
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Delete()
// 05sep2017(sar): Modified to use more secure CRT string functions. Had to add buffer size argument
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::GetValidFolder(LPTSTR Path, int bufSz)
{
    CString buffer = Path;
    BOOL valid = TRUE;
    int pos = -1;
    do {
        DWORD attrib = GetFileAttributes(buffer);
        if (attrib != 0xffffffff && (attrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            // the path is a valid folder
            if (buffer[buffer.GetLength() - 1] != '\\')
                buffer += "\\";
            _tcscpy_s(Path, bufSz, buffer);
            return valid;
        }
        valid = FALSE;
        pos = buffer.ReverseFind('\\');
        if (pos > 0)
        {
#ifdef _USING_VC60
           int len = buffer.GetLength();
           buffer.Delete(pos, len - pos);
#else
           CString strTemp = buffer.Left(pos);
           buffer = strTemp;
#endif // _USING_VC60
        }
    } while (pos > 0);

    // no valid folder, set 'Path' to empty string
    Path[0] = 0;
    return valid;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::ModifyFlags  (public member function)
//    Modifies the control flags
//
//  Parameters :
//    remove [in] - The flags to remove
//    add    [in] - The flags to add
//
//  Returns :
//    TRUE if the flags are successfully modified
//    FALSE if not
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::ModifyFlags(DWORD remove, DWORD add)
{
    DWORD Flags = GetFlags();
    Flags &= ~remove;
    Flags |= add;
    return SetFlags(Flags);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnChange  (protected member function)
//    Handles the EN_CHANGE message
//    Sets the m_bTextChanged flag
//
//  Parameters :
//    None
//
//  Returns :
//    FALSE
//
//  Note :
//    Returning FALSE allows the parent window to also handle the EN_CHANGE message
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::OnChange() 
{
    m_bTextChanged = TRUE;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnDropFiles  (protected member function)
//    Handles the WM_DROPFILES message
//    Sets the control text to display the files dropped onto the control
//
//  Parameters :
//    hDropInfo [in] - handle to the drop structure supplied by windows
//
//  Returns :
//    Nothing
//
//  Note :
//    The control must have the WS_EX_ACCEPTFILES extended windows
//    style bit set in order for drag and drop to work
//
// 19feb2001(saruffner):  Modified to also work under VC5.0, which does not implement CString::Delete()
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnDropFiles(HDROP hDropInfo) 
{
    int FolderLength = 0;
    CString szDroppedFiles = _T("");            // the new window text
    CString DropBuffer;                         // buffer to contain the dropped files
    CString szSeparator;
#if defined FEC_NORESOURCESTRINGS
    szSeparator = FEC_IDS_SEPARATOR;            // get the file separator character
#else
    szSeparator.LoadString(FEC_IDS_SEPARATOR);
#endif
    ASSERT(szSeparator.GetLength() == 1);       // must be one character only
    szSeparator += _T(" ");
    DWORD Flags = GetFlags();

    UINT nDropCount = DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);
    if (nDropCount && ((Flags & FEC_FOLDER) || ((Flags & FEC_FILE) && !(Flags & FEC_MULTIPLE))))
        // if (files dropped && (finding folder || (finding files && finding only one file)))
        nDropCount = 1;

    for (UINT x = 0; x < nDropCount; x++)
    {
        DragQueryFile(hDropInfo, x, DropBuffer.GetBuffer(_MAX_PATH), _MAX_PATH);
        DropBuffer.ReleaseBuffer();
        if ((Flags & FEC_FILE) && !(Flags & FEC_NODEREFERENCELINKS))
        {   // resolve any shortcut (*.lnk) files
            CString Ext = DropBuffer.Right(4);
            Ext.MakeLower();
            if (Ext = _T(".lnk"))
                DereferenceLink(DropBuffer);
        }
        if (x == 0)
        {   // first file has complete path, get the length of the path
            TCHAR Drive[_MAX_DRIVE];
            TCHAR Path[_MAX_PATH];
            TCHAR FName[_MAX_FNAME];
            TCHAR Ext[_MAX_EXT];
            _tsplitpath_s(DropBuffer, Drive, _MAX_DRIVE, Path, _MAX_PATH, FName, _MAX_FNAME, Ext, _MAX_EXT);
            FolderLength = static_cast<int>( _tcslen(Drive) + _tcslen(Path) );
        }
        else
        {   // all the rest of the files will drop the path if it
            // is the same as the first file's path
            if (DropBuffer.Left(FolderLength) == szDroppedFiles.Left(FolderLength))
            {
#ifdef _USING_VC60
               DropBuffer.Delete(0, FolderLength);
#else
               if( FolderLength >= DropBuffer.GetLength() ) DropBuffer.Empty();
               else
               {
                  CString strTemp = DropBuffer.Mid( FolderLength );
                  DropBuffer = strTemp;
               }
#endif // _USING_VC60
            }
            szDroppedFiles += szSeparator;
         }
         szDroppedFiles += DropBuffer;
    }

    DragFinish(hDropInfo);
    SetWindowText(szDroppedFiles);
    SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnEnable  (protected member function)
//    Handles the WM_ENABLE message
//    enables or disables the control, and redraws the button to reflect the new state
//
//  Parameters :
//    bEnable [in] - Enabled flag, TRUE to enable, FALSE to disable
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnEnable(BOOL bEnable) 
{
    CEdit::OnEnable(bEnable);
    DrawButton(bEnable ? BTN_UP : BTN_DISABLED);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnKeyDown  (protected member function)
//    Handles the WM_KEYDOWN message
//    Checks for the <CTRL> + 'period' keystroke, calls ButtonClicked() if found
//
//  Parameters :
//    nChar   [in] - The virtual key code
//    nRepCnt [in] - not used here, passed on to the base class
//    nFlags  [in] - not used here, passed on to the base class
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
#ifndef VK_OEM_PERIOD
#define VK_OEM_PERIOD 0xBE
#endif
    if ((nChar == VK_OEM_PERIOD || nChar == VK_DECIMAL) && GetKeyState(VK_CONTROL) < 0)
        ButtonClicked();
    else
        CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

/////////////////////////////////////////////////////////////////////////////
//
// Because the mouse is captured in OnNcLButtonDown(), we have to respond
// to WM_LBUTTONUP and WM_MOUSEMOVE messages.
// The m_bMouseCaptured variable is used because CEdit::OnLButtonDown()
// also captures the mouse, so using GetCapture() could give an invalid
// response.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnLButtonUp  (protected member function)
//    Handles the WM_LBUTTONUP message
//    Release the mouse capture and draw the button as normal. If the
//    cursor is over the button, call ButtonClicked() 
//
//  Parameters :
//    nFlags [in] - not used here, passed on to the base class
//    point  [in] - the location of the mouse cursor
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
    CEdit::OnLButtonUp(nFlags, point);

    if (m_bMouseCaptured)
    {
        ReleaseCapture();
        m_bMouseCaptured = FALSE;
        if (m_nButtonState != BTN_UP)
            DrawButton(BTN_UP);
        ClientToScreen(&point);
        if (ScreenPointInButtonRect(point))
            ButtonClicked();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnMouseMove  (protected member function)
//    Handles the WM_MOUSEMOVE message
//    If the mouse has been captured, draws the button as up or down
//    as the mouse moves on or off the button
//
//  Parameters :
//    nFlags [in] - not used here, passed on to the base class
//    point  [in] - The location of the mouse cursor
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
    CEdit::OnMouseMove(nFlags, point);

    if (m_bMouseCaptured)
    {
        ClientToScreen(&point);
        if (ScreenPointInButtonRect(point))
        {
            if (m_nButtonState != BTN_DOWN)
                DrawButton(BTN_DOWN);
        }
        else if (m_nButtonState != BTN_UP)
            DrawButton(BTN_UP);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnNcCalcSize  (protected member function)
//    Handles the WM_NCCALCSIZE message
//    Calculates the size and position of the button and client areas
//
//  Parameters :
//    bCalcValidRects [in] - specifies if the rgrc[1] and rgrc[2] rectangles in the 
//                           NCCALCSIZE_PARAMS structure are valid
//    lpncsp          [in] - Pointer to a NCCALCSIZE_PARAMS structure supplied by windows
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
    CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);

    // if m_nButtonWidth is -1, set the button width to 80% of the control height
    // else set the button width to m_nButtonWidth
    int ButtonWidth = (m_nButtonWidth == -1) ? (lpncsp->rgrc[0].bottom - lpncsp->rgrc[0].top) * 8/10 : m_nButtonWidth;
    
    // set button area equal to client area of edit control
    m_rcButtonRect = lpncsp->rgrc[0];
    
    if (m_bButtonLeft == FEC_BUTTONLEFT)  // draw the button on the left side of the control
    {   // shrink left side of client area by the button width
        lpncsp->rgrc[0].left += ButtonWidth;
        // shrink the button so its right side is at the left side of client area
        m_rcButtonRect.right = lpncsp->rgrc[0].left;
    }
    else   // draw the button on the right side of the control
    {   // shrink right side of client area by the button width
        // and add a one pixel neutral area between button and client area
        int neutral = ButtonWidth > 2 ? 1 : 0;
        lpncsp->rgrc[0].right -= ButtonWidth + neutral;
        // shrink the button so its left side is at the right side of client area
        m_rcButtonRect.left = lpncsp->rgrc[0].right + neutral;
    }
    if (bCalcValidRects)
        // convert button coordinates from parent client coordinates to control window coordinates
        m_rcButtonRect.OffsetRect(-lpncsp->rgrc[1].left, -lpncsp->rgrc[1].top);
    m_rcButtonRect.NormalizeRect();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnNcHitTest  (protected member function)
//    Handles the WM_NCHITTEST message
//    Ensures the control gets mouse messages when the mouse cursor is on the button
//
//  Parameters :
//    point [in] - The location of the mouse cursor
//
//  Returns :
//    The HitTest value for the mouse cursor location
//
//  Note :
//    If the mouse is over the button, OnNcHitTest() would normally return
//    HTNOWHERE, and we would not get any mouse messages. So we return 
//    HTBORDER to ensure we get them.
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CFileEditCtrl::OnNcHitTest(CPoint point) 
{
    LRESULT where = CEdit::OnNcHitTest(point);
    if (where == HTNOWHERE && ScreenPointInButtonRect(point))
        where = HTBORDER;
    return where;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnNcLButtonDblClk  (protected member function)
//    Handles the WM_NCLBUTTONDBLCLK message
//    Have a double click on the button be treated as two single clicks
//
//  Parameters :
//    nHitTest [in] - not used here, passed on to the base class
//    point    [in] - the location of the mouse cursor
//
//  Returns :
//    Nothing
//
//  Note :
//    When this control sends a FEC_NM_PREBROWSE notification message to its
//    parent window, and the parent window returns a nonzero value, the browse
//    dialogs do not execute. This function makes the button go down and up on
//    the second click of a double click
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnNcLButtonDblClk(UINT nHitTest, CPoint point) 
{
    if (ScreenPointInButtonRect (point))
        OnNcLButtonDown(nHitTest, point);
    else
        CEdit::OnNcLButtonDblClk(nHitTest, point);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnNcLButtonDown  (protected member function)
//    Handles the WM_NCLBUTTONDOWN message
//    If the user clicks on the button, capture mouse input, set the focus
//    to this control, and draw the button as pressed
//
//  Parameters :
//    nHitTest [in] - not used here, passed on to the base class
//    point    [in] - the location of the mouse cursor
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
    CEdit::OnNcLButtonDown(nHitTest, point);

    if (ScreenPointInButtonRect(point))
    {
        SetFocus();
        SetCapture();
        m_bMouseCaptured = TRUE;
        DrawButton(BTN_DOWN);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnNcPaint  (protected member function)
//    Handles the WM_NCPAINT message
//    Redraws the control as needed
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnNcPaint() 
{
    CEdit::OnNcPaint();             // draws the border around the control
    DrawButton(m_nButtonState);     // draw the button in its current state
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnSetFocus  (protected member function)
//    Handles the WM_SETFOCUS message
//    Selects (hilites) all the text in the control
//
//  Parameters :
//    pOldWnd [in] - not used here, passed on to the base class
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnSetFocus(CWnd* pOldWnd) 
{
    CEdit::OnSetFocus(pOldWnd);
    SetSel(0, -1);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnSetText  (protected member function)
//    Handles the WM_SETTEXT message
//    Sets the m_bTextChanged flag
//
//  Parameters :
//    None
//
//  Returns :
//    Whatever the default windows procedure returns
//
//  Note :
//    OnChange() does not seem to get called every time a WM_SETTEXT message
//    is sent to this control, so I duplicated it's functionality here.
//
//    CWnd does not have an OnSetText() handler function, so I called Default()
//    to ensure this message is properly handled 
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CFileEditCtrl::OnSetText(WPARAM, LPARAM)
{
    m_bTextChanged = TRUE;
    return Default();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnSize  (protected member function)
//    Handles the WM_SIZE message
//    Forces a recalculation of the button's and client area's size and position
//    also recalculates the tooltips bounding rectangles
//
//  Parameters :
//    nType [in] - not used here, passed on to the base class
//    cx    [in] - not used here, passed on to the base class
//    cy    [in] - not used here, passed on to the base class
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::OnSize(UINT nType, int cx, int cy) 
{
    CEdit::OnSize(nType, cx, cy);

    SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if (IsWindow (m_ToolTip))
    {
        // recalculate the bounding rectangle for the button tooltip
        CRect rcBtn(m_rcButtonRect);
        CRect rcWnd;
        GetWindowRect(&rcWnd);
        rcBtn.OffsetRect(rcWnd.left, rcWnd.top);
        ScreenToClient(&rcBtn);
        m_ToolTip.SetToolRect(this, ID_BUTTONTIP, &rcBtn);

        // recalculate the bounding rectangle for the client area tooltip
        GetClientRect(&rcWnd);
        m_ToolTip.SetToolRect(this, ID_CLIENTTIP, &rcWnd);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::OnTTNNeedText  (protected member function)
//    Handles the TTN_NEEDTEXT message from the tooltip control
//    Sets the tooltip text
//
//  Parameters :
//    pTTTStruct [in] - pointer to a TOOLTIPTEXT structure
//
//  Returns :
//    TRUE
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::OnTTNNeedText(UINT, NMHDR *pTTTStruct, LRESULT *)
{
    DWORD Flags = GetFlags();
    TOOLTIPTEXT* pTTT = ((TOOLTIPTEXT*)pTTTStruct);
    if ((Flags & FEC_BUTTONTIP) && pTTT->hdr.idFrom == ID_BUTTONTIP)
    {
#if defined FEC_NORESOURCESTRINGS
        pTTT->lpszText = FEC_IDS_BUTTONTIP;
#else
        pTTT->lpszText = MAKEINTRESOURCE(FEC_IDS_BUTTONTIP);
        pTTT->hinst = AfxGetResourceHandle();
#endif
    }
    if ((Flags & FEC_CLIENTTIP) && pTTT->hdr.idFrom == ID_CLIENTTIP)
        pTTT->lpszText = m_szClientTip.GetBuffer(m_szClientTip.GetLength());
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::PostNcDestroy  (protected member function)
//    deletes this control object if the m_bAutoDelete flag is set
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::PostNcDestroy() 
{
    if (m_bAutoDelete)
        delete this;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::PreTranslateMessage  (public member function)
//    Sets up and passes messages to the tooltip control
//
//  Parameters :
//    pMsg [in] - the current windows message
//
//  Returns :
//    whatever CEdit::PreTranslateMessage() returns
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::PreTranslateMessage(MSG* pMsg) 
{   // not using GetFlags() because this is faster
    if (m_dwFlags & (FEC_BUTTONTIP | FEC_CLIENTTIP))
    {
        if (!IsWindow (m_ToolTip))
        {
            // create and activate the tooltip control
            m_ToolTip.Create(this);
            m_ToolTip.Activate(TRUE);
            m_ToolTip.SetFont(GetFont());

            // Setup the button tooltip
            CRect rc(m_rcButtonRect);
            CRect wnd;
            GetWindowRect(&wnd);
            rc.OffsetRect(wnd.left, wnd.top);
            ScreenToClient(&rc);
            m_ToolTip.AddTool(this, LPSTR_TEXTCALLBACK, &rc, ID_BUTTONTIP);

            // Setup the client area tooltip
            GetClientRect(&wnd);
            m_ToolTip.AddTool(this, LPSTR_TEXTCALLBACK, &wnd, ID_CLIENTTIP);
        }
        m_ToolTip.RelayEvent(pMsg);
    }

    return CEdit::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::ScreenPointInButtonRect  (protected member function)
//    determine if the mouse cursor is on the button
//
//  Parameters :
//    point [in] - The location of the mouse cursor in screen coordinates
//
//  Returns :
//    TRUE if the mouse cursor is on the button
//    FALSE if it is not
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::ScreenPointInButtonRect(CPoint point)
{
    CRect ControlRect;
    GetWindowRect(&ControlRect);
    // convert point from screen coordinates to window coordinates
    point.Offset(-ControlRect.left, -ControlRect.top);
    return m_rcButtonRect.PtInRect(point);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::SetBrowseImage  (public member function)
//    Set the image to be used on the browse button
//
//  Parameters :
//    pButtonImage [in] - a pointer to a CFECButtonImage for the image
//
//  Returns :
//    Nothing
//
//  Note :
//    If pButtonImage is NULL, the ellipses will be drawn on the button
//    If pButtonImage.m_hImage is NULL, the button will be blank
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::SetButtonImage(CFECButtonImage *pButtonImage)
{
    m_pButtonImage = pButtonImage;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::SetButtonWidth  (public member function)
//    Sets the width, in pixels, of the browse button
//
//  Parameters :
//    width [in] - The new width of the button
//
//  Returns :
//    The previous width of the button
//
//  Note :
//    Setting the width to -1 causes the control to use the default width
//    which is calculated to be 80% of it's height
//
/////////////////////////////////////////////////////////////////////////////

int CFileEditCtrl::SetButtonWidth(int width/* = -1*/)
{
    int oldWidth = GetButtonWidth();
    m_nButtonWidth = width;
    SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    return oldWidth;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::SetClientTipText  (public member function)
//    Sets the text to be used by the client area tooltip
//
//  Parameters :
//    text [in] - The text to set
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFileEditCtrl::SetClientTipText(CString text)
{
    m_szClientTip = text;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFileEditCtrl::SetFlags  (public member function)
//    Sets all the internal flags
//    Initializes and sets up the OPENFILENAME or BROWSEINFO structures
//    Forces the control to be redrawn if the button position changes
//
//  Parameters :
//    dwFlags [in] - The flags to set
//
//  Returns :
//    TRUE if successful
//    FALSE if not
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFileEditCtrl::SetFlags(DWORD dwFlags)
{
    m_bTextChanged = TRUE;

    if (dwFlags & FEC_FOLDER)
    {   // set the control to find folders
        if (dwFlags & FEC_FILE)
        {
            TRACE (_T("CFileEditCtrl::SetFlags() : Can not specify FEC_FILE with FEC_FOLDER\n"));
            ASSERT (FALSE);
            return FALSE;
        }
        if (!m_pBROWSEINFO)
        {   // create the BROWSEINFO structure
            m_pBROWSEINFO = new BROWSEINFO;
            if (!m_pBROWSEINFO)
            {
                TRACE (_T("CFileEditCtrl::SetFlags() : Failed to create the BROWSEINFO structure\n"));
                ASSERT (FALSE);
                return FALSE;
            }
            if (m_pCFileDialog)
            {
                delete m_pCFileDialog;          // delete the CFileDialog
                m_pCFileDialog = NULL;
            }
            // set up the BROWSEINFO structure used by SHBrowseForFolder()
            ::ZeroMemory(m_pBROWSEINFO, sizeof(BROWSEINFO));
            m_pBROWSEINFO->hwndOwner = GetSafeHwnd();
            m_pBROWSEINFO->lParam = (LPARAM)this;
            m_pBROWSEINFO->lpfn = FECFolderProc;
            m_pBROWSEINFO->ulFlags = BIF_RETURNONLYFSDIRS;
        }
    }

    else if (dwFlags & FEC_FILE)
    {   // set the control to find files
        if (m_pCFileDialog)
        {   // already set to find files
            if (dwFlags & FEC_MULTIPLE)
                m_pCFileDialog->m_ofn.Flags |= OFN_ALLOWMULTISELECT;
            else
                m_pCFileDialog->m_ofn.Flags &= ~OFN_ALLOWMULTISELECT;
            
            if (dwFlags & FEC_NODEREFERENCELINKS)
                m_pCFileDialog->m_ofn.Flags |= OFN_NODEREFERENCELINKS;
            else
                m_pCFileDialog->m_ofn.Flags &= ~OFN_NODEREFERENCELINKS;
        }
        else
        {   // create the CFileDialog
            CString szFilter;
#if defined FEC_NORESOURCESTRINGS
            szFilter = FEC_IDS_ALLFILES;
#else
            szFilter.LoadString(FEC_IDS_ALLFILES);
#endif
            m_pCFileDialog = new CFECFileDialog(TRUE,
                NULL,
                NULL,
                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
                szFilter,
                this);
            if (!m_pCFileDialog)
            {
                TRACE (_T("CFileEditCtrl::SetFlags() : Failed to create the CFileDialog\n"));
                ASSERT (FALSE);
                return FALSE;
            }
            if (m_pBROWSEINFO)
            {
                delete m_pBROWSEINFO;               // delete the BROWSEINFO structure
                m_pBROWSEINFO = NULL;
            }
            // set up the CFileDialog
            if (dwFlags & FEC_MULTIPLE)
                m_pCFileDialog->m_ofn.Flags |= OFN_ALLOWMULTISELECT;
            if (dwFlags & FEC_NODEREFERENCELINKS)
                m_pCFileDialog->m_ofn.Flags |= OFN_NODEREFERENCELINKS;

            m_pCFileDialog->m_ofn.hwndOwner = GetSafeHwnd();
#if defined FEC_NORESOURCESTRINGS
            m_szCaption = FEC_IDS_FILEDIALOGTITLE;
#else
            m_szCaption.LoadString(FEC_IDS_FILEDIALOGTITLE);
#endif
            m_pCFileDialog->m_ofn.lpstrTitle = (LPCTSTR)m_szCaption;
        }
    }

    else
    {
        TRACE (_T("CFileEditCtrl::SetFlags() : Must specify either FEC_FILE or FEC_FOLDER when setting flags\n"));
        ASSERT (FALSE);
        return FALSE;
    }

    // m_bButtonLeft will be 0xFFFFFFFF the first time SetFlags() is called
    // (dwFlags & FEC_BUTTONLEFT) will be either 0x00000000 or FEC_BUTTONLEFT
    if (m_bButtonLeft != (dwFlags & FEC_BUTTONLEFT))
    {
        // move the button to the desired side of the control
        m_bButtonLeft = dwFlags & FEC_BUTTONLEFT;
        // force a call to OnNcCalcSize to calculate the size and position of the button and client area
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    m_dwFlags = dwFlags;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DDV_FileEditCtrl & DDX_FileEditCtrl

/////////////////////////////////////////////////////////////////////////////
//
//  DDV_FileEditCtrl  (Global function)
//    Verifies that the files or folders entered actually exist
//
//  Parameters :
//    pDX  [in] - Pointer to the CDataExchange object
//    nIDC [in] - The controls resource ID
//
//  Returns :
//    Nothing
//
//  Note :
//    If the file or folder is invalid, pops up a messagebox informing the user,
//    then sets the focus to the offending CFileEditCtrl
//
/////////////////////////////////////////////////////////////////////////////

void DDV_FileEditCtrl(CDataExchange *pDX, int nIDC)
{
    CWnd *pWnd = pDX->m_pDlgWnd->GetDlgItem(nIDC);
    ASSERT(pWnd);
    if (!pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)))  // is this control a CFileEditCtrl control?
    {
        TRACE(_T("Control %d not subclassed to CFileEditCtrl, must first call DDX_FileEditCtrl()"), nIDC);
        ASSERT(FALSE);
        AfxThrowNotSupportedException();
    }
    if (!pDX->m_bSaveAndValidate)               // not saving data
        return;
    CFileEditCtrl *pFEC = (CFileEditCtrl *)pWnd;
    pDX->PrepareEditCtrl(nIDC);
    POSITION pos = pFEC->GetStartPosition();
    if (!pos)
    {	// no name entered
        AfxMessageBox(FEC_IDS_NOFILE);
        pDX->Fail();
    }
    while (pos)
    {
        CString szMessage;
        CString szFile = pFEC->GetNextPathName(pos);
        DWORD dwAttribute = GetFileAttributes(szFile);
        if (dwAttribute == 0xFFFFFFFF)          // GetFileAttributes() failed
        {                                       // does not exist
            szMessage.Format(FEC_IDS_NOTEXIST, szFile);
            AfxMessageBox(szMessage);
            pDX->Fail();
        }
        if ((pFEC->GetFlags() & FEC_FOLDER) && !(dwAttribute & FILE_ATTRIBUTE_DIRECTORY))
        {                                       // not a folder
            szMessage.Format(FEC_IDS_NOTFOLDER, szFile);
            AfxMessageBox(szMessage);
            pDX->Fail();
        }
        if ((pFEC->GetFlags() & FEC_FILE) && (dwAttribute & FILE_ATTRIBUTE_DIRECTORY))
        {                                       // not a file
            szMessage.Format(FEC_IDS_NOTFILE, szFile);
            AfxMessageBox(szMessage);
            pDX->Fail();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  DDX_FileEditCtrl (Global function)
//    Subclasses the control with the given ID
//    Transfers the data between the control and the supplied CString
//
//  Parameters :
//    pDX     [in] - Pointer to the CDataExchange object
//    nIDC    [in] - The controls resource ID
//    rStr    [in] - The CString that contains the initial control text, and receives the text
//    dwFlags [in] - The flags used to setup this control
//
//  Returns :
//    Nothing
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//    the FEC_MULTIPLE flag can not be used (how can multiple files be returned in one CString?)
//
/////////////////////////////////////////////////////////////////////////////

void DDX_FileEditCtrl(CDataExchange *pDX, int nIDC, CString& rStr, DWORD dwFlags)
{
    CWnd *pWnd = pDX->m_pDlgWnd->GetDlgItem(nIDC);
    ASSERT(pWnd);
    if (pDX->m_bSaveAndValidate)                // update string with text from control
    {
        // ensure the control is a CFileEditCtrl control
        ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)));
        // copy the first file listed in the control to the string
        rStr.Empty();
        CFileEditCtrl *pFEC = (CFileEditCtrl *)pWnd;
        POSITION pos = pFEC->GetStartPosition();
        if (pos)
            rStr = pFEC->GetNextPathName(pos);
    }
    else                                        // create the control if it is not already created
    {                                           // set the control text to the text in string
        CFileEditCtrl *pFEC = NULL;
        if (!pWnd->IsKindOf(RUNTIME_CLASS(CFileEditCtrl)))    // not subclassed yet
        {
            // create then subclass the control to the edit control with the ID nIDC
            HWND hWnd = pDX->PrepareEditCtrl(nIDC);
            pFEC = new CFileEditCtrl(TRUE);     // create the control with autodelete
            if (!pFEC->SubclassWindow(hWnd))
            {                                   // failed to subclass the edit control
                ASSERT(FALSE);
                AfxThrowNotSupportedException();
            }
            // call CFileEditCtrl::SetFlags() to initialize the internal data structures
            dwFlags &= ~FEC_MULTIPLE;           // can not put multiple files in one CString
            if (!pFEC->SetFlags(dwFlags))
            {
                ASSERT(FALSE);
                AfxThrowNotSupportedException();
            }
        }
        else                                    // control already exists
            pFEC = (CFileEditCtrl *)pWnd;
        if (pFEC)
            pFEC->SetWindowText(rStr);          // set the control text
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  DDX_FileEditCtrl (Global function)
//    Subclasses the control with the given ID
//    Transfers the data between the window text and the supplied CFileEditCtrl
//
//  Parameters :
//    pDX     [in] - Pointer to the CDataExchange object
//    nIDC    [in] - The controls resource ID
//    rCFEC   [in] - The CFileEditCtrl object that is to control this window
//    dwFlags [in] - The flags used to setup this control
//
//  Returns :
//    Nothing
//
//  Note :
//    See the FileEditCtrl.h file for descriptions of the flags used
//
/////////////////////////////////////////////////////////////////////////////

void DDX_FileEditCtrl(CDataExchange *pDX, int nIDC, CFileEditCtrl &rCFEC, DWORD dwFlags)
{
    ASSERT(pDX->m_pDlgWnd->GetDlgItem(nIDC));
    if (rCFEC.m_hWnd == NULL)                   // not yet subclassed
    {
        ASSERT(!pDX->m_bSaveAndValidate);
        // subclass the control to the edit control with the ID nIDC
        HWND hWnd = pDX->PrepareEditCtrl(nIDC);
        if (!rCFEC.SubclassWindow(hWnd))
        {                                       // failed to subclass the edit control
            ASSERT(FALSE);
            AfxThrowNotSupportedException();
        }
        // call CFileEditCtrl::SetFlags() to initialize the internal data structures
        if (!rCFEC.SetFlags(dwFlags))
        {
            ASSERT(FALSE);
            AfxThrowNotSupportedException();
        }
    }
    else if (pDX->m_bSaveAndValidate)
        rCFEC.GetStartPosition();               // updates the data from the edit control
}

/////////////////////////////////////////////////////////////////////////////
// FEC_NOTIFY structure

/////////////////////////////////////////////////////////////////////////////
//
//  FEC_NOTIFY constructor (public member function)
//    Initializes the FEC_NOTIFY structure used when the CFileEditCtrl sends
//    a WM_NOTIFY message to it's parent window (in CFileEditCtrl::ButtonClicked())
//
//  Parameters :
//    FEC  [in] - pointer to the CFileEditCtrl sending the message
//    code [in] - The notification message being sent. (FEC_NM_PREBROWSE or FEC_NM_POSTBROWSE)
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

tagFEC_NOTIFY::tagFEC_NOTIFY (CFileEditCtrl *FEC, UINT code)
{
    pFEC = FEC;
    hdr.hwndFrom = FEC->GetSafeHwnd();
    hdr.idFrom = FEC->GetDlgCtrlID();
    hdr.code = code;
}

/////////////////////////////////////////////////////////////////////////////
// CFECFileDialog
// 07feb2013(sar): HACK to work-around bug in new Vista-style file dialog. The underlying
// shell dialog implementation, IFileDialog, will assert if SetFileTypes is called more
// than once on an instance. Since CFileEditCtrl reuses the file dialog object, this will
// usually happen the second time the dialog is raised. To WORKAROUND the bug, we disable
// the Vista dialog style using the VS2008 version of the CFileDialog() constructor...
IMPLEMENT_DYNAMIC(CFECFileDialog, CFileDialog)

CFECFileDialog::CFECFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                               DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, 0, FALSE)
{
}


BEGIN_MESSAGE_MAP(CFECFileDialog, CFileDialog)
//{{AFX_MSG_MAP(CFECFileDialog)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
//  CFECFileDialog::OnInitDialog  (protected member function)
//    Set the text of the IDOK button on an old style dialog to 'OK'
//
//  Parameters :
//    None
//
//  Returns :
//    TRUE
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFECFileDialog::OnInitDialog() 
{
    CFileDialog::OnInitDialog();
    
    if (!(m_ofn.Flags & OFN_EXPLORER))
    {
        CString szOkButton;
#if defined FEC_NORESOURCESTRINGS
        szOkButton = FEC_IDS_OKBUTTON;
#else
        szOkButton.LoadString(FEC_IDS_OKBUTTON);
#endif
        GetDlgItem(IDOK)->SetWindowText(szOkButton);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECFileDialog::OnInitDone  (protected member function)
//    Set the text of the IDOK button on an explorer style dialog to 'OK'
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CFECFileDialog::OnInitDone()
{
    CString szOkButton;
#if defined FEC_NORESOURCESTRINGS
    szOkButton = FEC_IDS_OKBUTTON;
#else
    szOkButton.LoadString(FEC_IDS_OKBUTTON);
#endif
    CommDlg_OpenSave_SetControlText(GetParent()->m_hWnd, IDOK, (LPCTSTR)szOkButton);
}


/////////////////////////////////////////////////////////////////////////////
// CFECButtonImage class

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage constructor  (public member function)
//    Initializes member variables
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

CFECButtonImage::CFECButtonImage()
{
    m_Flags = 0;
    m_hImage = NULL;
    m_size.cx = 0;
    m_size.cy = 0;
    m_TransparentColour = CLR_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage destructor  (public member function)
//    frees the memory held by the image handle
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

CFECButtonImage::~CFECButtonImage()
{
    if (m_hImage)
    {
        if (m_Flags & FEC_ICON)
            DestroyIcon((HICON)m_hImage);
        else
            DeleteObject((HGDIOBJ)m_hImage);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::DitherBlt  (protected member function)
//    Draws the image on the FromDC as a disabled (grayed) image onto the pToDC
//
//  Parameters :
//    pToDC  [in] - pointer to the DC to draw the bitmap onto
//    x      [in] - the left side of the image on the destination DC
//    y      [in] - the top  of the image on the destination DC
//    w      [in] - the width of the image on the destination DC
//    h      [in] - the height of the image on the destination DC
//    FromDC [in] - The DC containing the bitmap to be grayed
//
//  Returns :
//    Nothing
//
//  Note : modified from code found at http://www.codeguru.com/bitmap/dither_blt.shtml
//         original author Jean-Edouard Lachand-Robert (iamwired@geocities.com)
//
/////////////////////////////////////////////////////////////////////////////

void CFECButtonImage::DitherBlt(CDC *pToDC, int x, int y, int w, int h, CDC *pFromDC)
{
    CDC MonoDC;
    if (MonoDC.CreateCompatibleDC(pToDC))
    {
        struct {
            BITMAPINFOHEADER bmiHeader; 
            RGBQUAD          bmiColors[2]; 
        } RGBBWBITMAPINFO = { { sizeof(BITMAPINFOHEADER),
            w,
            h,
            1,
            1,
            BI_RGB,
            0,
            0,
            0,
            0,
            0},
        { { 0x00, 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 }}};
        VOID *pbitsBW;
        HBITMAP hbmBW = CreateDIBSection(MonoDC.m_hDC,
            (LPBITMAPINFO)&RGBBWBITMAPINFO,
            DIB_RGB_COLORS,
            &pbitsBW,
            NULL,
            0);
        ASSERT(hbmBW);

        if (hbmBW)
        {
            int SavedMonoDC = MonoDC.SaveDC();
            int SavedpToDC = pToDC->SaveDC();

            // Attach the monochrome DIB section and the bitmap to the DCs
            MonoDC.SelectObject(hbmBW);
            
            // BitBlt the bitmap into the monochrome DIB section
            MonoDC.BitBlt(0, 0, w, h, pFromDC, 0, 0, SRCCOPY);
            
            // BitBlt the black bits in the monochrome bitmap into COLOR_3DHILIGHT bits in the destination DC
            // The magic ROP comes from the Charles Petzold's book
            HBRUSH hb = CreateSolidBrush(GetSysColor(COLOR_3DHILIGHT));
            pToDC->SelectObject(hb);
            pToDC->BitBlt(x + 1, y + 1, w, h, &MonoDC, 0, 0, 0x00B8074A);
            
            // BitBlt the black bits in the monochrome bitmap into COLOR_3DSHADOW bits in the destination DC
            hb = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
            DeleteObject(pToDC->SelectObject(hb));
            pToDC->BitBlt(x, y, w, h, &MonoDC, 0, 0, 0x00B8074A);
            
            pToDC->RestoreDC(SavedpToDC);
            MonoDC.RestoreDC(SavedMonoDC);
            DeleteObject(hb);
        }
        DeleteObject(hbmBW);
        MonoDC.DeleteDC();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::DrawImage  (public member function)
//    Draws the image (set with the SetImage() function) on the given device 
//    context
//
//  Parameters :
//    pDC      [in] - a pointer to the the device context to draw the image on
//    x        [in] - the left side of the image on the destination DC
//    y        [in] - the top  of the image on the destination DC
//    w        [in] - the width of the image on the destination DC
//    h        [in] - the height of the image on the destination DC
//    Disabled [in] - Draw the image disabled (grayed)
//
//  Returns :
//    Nothing
//
//  Note :
//    If the image is an icon or a transparent bitmap, the brush that is
//    selected into the pDC will be used as the background. The brush can 
//    be a pattern or bitmap brush.
//
//    The image will be drawn entirely within the rectangle specified by the
//    x, y, w, and h parameters.
//
//    If the FEC_STRETCH flag is set (See SetImage()) the image will be
//    stretched (or shrunk) so that the entire image fills the aforementioned
//    rectangle. If the FEC_STRETCH flag is not set, the image will be
//    centered on the rectangle. If the image is bigger than the rectangle
//    it will be clipped on all sides.
//
//    If the image is a bitmap, and the FEC_TRANSPARENT flag is set, the 
//    colour set as the transparent colour (see SetTransparentColour())
//    will be removed from the bitmap.
//
/////////////////////////////////////////////////////////////////////////////

void CFECButtonImage::DrawImage(CDC *pDC, int x, int y, int w, int h, BOOL Disabled /*= FALSE*/)
{   // sanity check
    if (!m_hImage)
        return;

    // first we set the clip region to the specified rectangle
    CRgn ClipRgn;
    ClipRgn.CreateRectRgn(x, y, x + w, y + h);
    pDC->SelectClipRgn(&ClipRgn);
    ClipRgn.DeleteObject();

    // center the image on the output rectangle
    int left = x + (w / 2) - (m_size.cx / 2);
    int top = y + (h / 2) - (m_size.cy / 2);
    int width = m_size.cx;
    int height = m_size.cy;

    // create memory DC
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    int savedmemDC = memDC.SaveDC();

    // create bitmap for memDC
    CBitmap memDCBmp;

    // Create a DC and bitmap for the stretched image
    CDC StretchDC;
    int savedStretchDC = 0;
    CBitmap stretchbmp;
    if (m_Flags & FEC_STRETCH)
    {   // stretch image to fit output rectangle
        left = x;
        top = y;
        width = w;
        height = h;

        // setup the stretch DC
        StretchDC.CreateCompatibleDC(pDC);
        savedStretchDC = StretchDC.SaveDC();

        stretchbmp.CreateCompatibleBitmap(pDC, w, h);
        StretchDC.SelectObject(stretchbmp);
        StretchDC.SetStretchBltMode(COLORONCOLOR);
    }

    // get the brush origins in case we are using a bitmap or pattern brush
    CPoint BrushOrg;
    CPoint Org = pDC->GetBrushOrg();
    div_t divt = div((int) (left - Org.x), (int)8);
    BrushOrg.x = 8 - divt.rem;
    divt = div((int) (top - Org.y), (int)8);
    BrushOrg.y = 8 - divt.rem;

    // Create a DC and bitmap for the transparent image
    CDC TransparentDC;
    int savedTransparentDC = 0;
    CBitmap Transparentbmp;
    if ((m_Flags & FEC_BITMAP) && (m_Flags & FEC_TRANSPARENT))
    {   // setup the transparent DC
        TransparentDC.CreateCompatibleDC(pDC);
        savedTransparentDC = TransparentDC.SaveDC();

        Transparentbmp.CreateCompatibleBitmap(pDC, width, height);
        TransparentDC.SelectObject(&Transparentbmp);
        TransparentDC.SetBrushOrg(BrushOrg);
        TransparentDC.FillRect(CRect(0, 0, width + 1, height + 1), pDC->GetCurrentBrush());
    }

    CDC* pOutputDC = &memDC;

    if (m_Flags & FEC_ICON)
    {   // draw the icon onto the memory DC
        memDCBmp.CreateCompatibleBitmap(pDC, width, height);
        memDC.SelectObject(&memDCBmp);
        memDC.SetBrushOrg(BrushOrg);
        memDC.FillRect(CRect(0, 0, width + 1, height + 1), pDC->GetCurrentBrush());
        DrawIconEx(memDC.m_hDC, 0, 0, (HICON)m_hImage, width, height, 0, NULL, DI_NORMAL);
    }
    else if (m_Flags & FEC_BITMAP)
    {   // place bitmap image into the memory DC
        memDC.SelectObject((HBITMAP)m_hImage);

        if (m_Flags & FEC_STRETCH)
        {   // stretch the image
            StretchDC.StretchBlt(0, 0, width, height, &memDC, 0, 0, m_size.cx, m_size.cy, SRCCOPY);
            pOutputDC = &StretchDC;
        }

        if (m_Flags & FEC_TRANSPARENT)
        {   // draw the image transparently
            DrawTransparent(&TransparentDC, 0, 0, width, height, pOutputDC);
            pOutputDC = &TransparentDC;
        }
    }
    else
    {
        ASSERT (FALSE);  // m_Flags improperly set (should never get here)
    }

    if (Disabled)  // draw the image grayed
        DitherBlt(pDC, left, top, width, height, pOutputDC);

    else   // draw the image
        pDC->BitBlt(left, top, width, height, pOutputDC, 0, 0, SRCCOPY);

    // clean up after ourselves
    if (savedTransparentDC)
    {
        TransparentDC.RestoreDC(savedTransparentDC);
        TransparentDC.DeleteDC();
    }

    if (savedStretchDC)
    {
        StretchDC.RestoreDC(savedStretchDC);
        StretchDC.DeleteDC();
    }

    memDC.RestoreDC(savedmemDC);
    memDC.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::DrawTransparent  (protected member function)
//    transparently draws the image in the source device context onto the 
//    destination device context 
//
//  Parameters :
//    pToDC  [in] - pointer to the destination device context
//    x      [in] - the left side of the image on the destination DC
//    y      [in] - the top  of the image on the destination DC
//    w      [in] - the width of the image on the destination DC
//    h      [in] - the height of the image on the destination DC
//    FromDC [in] - the source DC containing the bitmap to be drawn
//
//  Returns :
//    Nothing
//
//  Note :
//    Uses the 'True Mask' method
//    Modified from code found at http://www.codeguru.com/bitmap/CISBitmap.shtml
//    original author Paul Reynolds (Paul.Reynolds@cmgroup.co.uk)
// 
/////////////////////////////////////////////////////////////////////////////

void CFECButtonImage::DrawTransparent(CDC *pToDC, int x, int y, int w, int h, CDC *pFromDC)
{
    CDC MonoDC;
    MonoDC.CreateCompatibleDC(pToDC);

    int savedToDC = pToDC->SaveDC();
    int savedFromDC = pFromDC->SaveDC();
    int savedMonoDC = MonoDC.SaveDC();

    COLORREF colourTransparent = m_TransparentColour;
    if (m_TransparentColour == CLR_DEFAULT)
        colourTransparent = pFromDC->GetPixel(0,0);

    pToDC->SetBkColor(RGB(255, 255, 255));
    pToDC->SetTextColor(RGB(0, 0, 0));
    pFromDC->SetBkColor(colourTransparent);

    CBitmap MonoDCbmp;
    MonoDCbmp.CreateBitmap(w, h, 1, 1, NULL);

    MonoDC.SelectObject(&MonoDCbmp);
    MonoDC.BitBlt(0, 0, w, h, pFromDC, 0, 0, SRCCOPY);

    pToDC->BitBlt(x, y, w, h, pFromDC, 0, 0, SRCINVERT);
    pToDC->BitBlt(x, y, w, h, &MonoDC, 0, 0, SRCAND);
    pToDC->BitBlt(x, y, w, h, pFromDC, 0, 0, SRCINVERT);

    MonoDC.RestoreDC(savedMonoDC);
    pFromDC->RestoreDC(savedFromDC);
    pToDC->RestoreDC(savedToDC);
    MonoDC.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::GetSize  (public member function)
//    Gets the size of the image in pixels
//
//  Parameters :
//    None
//
//  Returns :
//    a CSize containing the size of the image
//
/////////////////////////////////////////////////////////////////////////////

CSize CFECButtonImage::GetSize()
{
    return m_size;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::SetImage  (public member function)
//    Sets the image and image flags
//
//  Parameters :
//    Image [in] - a HANDLE of the image to set (either a HBITMAP or a HICON)
//    Flags [in] - the flags that specify the type of image and how it is drawn
//
//  Returns :
//    TRUE on success
//    FALSE on failure
//
//  Note :
//    See the CFileEditCtrl.h file for a description of the flags used
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFECButtonImage::SetImage(HANDLE Image, DWORD Flags)
{
    if (Image)
    {   // verify flags
        if (!((Flags & FEC_BITMAP) ^ (Flags & FEC_ICON)))
        {
            TRACE (_T("FEC_ButtonImage::SetImage() : Must specify either FEC_BITMAP or FEC_ICON"));
            ASSERT (FALSE);
            return FALSE;
        }
    }

    if (m_hImage && m_hImage != Image)
    {   // remove the old image
        if (m_Flags & FEC_ICON)
            DestroyIcon((HICON)m_hImage);
        else
            DeleteObject((HGDIOBJ)m_hImage);
        m_hImage = NULL;
        m_size.cx = 0;
        m_size.cy = 0;
    }

    if (Image)
    {   // get the image dimensions
        if (Flags & FEC_BITMAP)
        {
            BITMAP bmp;
            if (GetObject((HBITMAP)Image, sizeof(BITMAP), &bmp))
            {
                m_size.cx = bmp.bmWidth;
                m_size.cy = bmp.bmHeight;
            }
        }
        else if (Flags & FEC_ICON)
        {
            ICONINFO iconinfo;
            GetIconInfo((HICON)Image, &iconinfo);
            BITMAP bmp;
            if (GetObject(iconinfo.hbmMask, sizeof(BITMAP), &bmp))
            {
                m_size.cx = bmp.bmWidth;
                m_size.cy = iconinfo.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2;
            }
            // prevent a resource leak
            DeleteObject(iconinfo.hbmColor);
            DeleteObject(iconinfo.hbmMask);
        }
    }

    m_hImage = Image;
    m_Flags = Flags;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CFECButtonImage::SetTransparentColour  (public member function)
//    Set the colour to be used as the transparent colour 
//
//  Parameters :
//    clr [in] - the colour to be used as the transparent colour
//
//  Returns :
//    the old transparent colour
//
//  Note :
//    This function only works if the image is a HBITMAP and the FEC_TRANSPARENT
//    flag is set. If the colour is CLR_DEFAULT (the default), the colour of the
//    top left pixel (0,0) is used as the transparent colour.
//
/////////////////////////////////////////////////////////////////////////////

COLORREF CFECButtonImage::SetTransparentColour(COLORREF clr /*= CLR_DEFAULT*/)
{
    COLORREF oldclr = m_TransparentColour;
    m_TransparentColour = clr;
    return oldclr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  End of FileEditCtrl.cpp
//
/////////////////////////////////////////////////////////////////////////////
