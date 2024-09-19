//===================================================================================================================== 
//
// dirchooser.cpp : Implementation of CDirChooser, an encapsulation of the Windows shell function SHBrowseForFolder().
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// This is a very simple encapsulation of the SHBrowseForFolder(), which lets the user browse the file system and 
// select a directory.  CDirChooser limits browsing to file system folders, even though SHBrowseForFolder() can search 
// for other resources (such as printers, networked devices, etc.).  
//
// ==> Usage:  Construct an instance and invoke Browse(), optionally specifying a file system folder at which browsing 
// should start (if not specified, CDirChooser tries to start at the system temp folder).  When Browse() returns TRUE, 
// retrieve the pathname of the folder selected by calling GetChosenDirectory().  Note that CDirChooser "remembers" the 
// folder chosen in the last browse operation, so the next browse will start there if no initial directory is specified 
// in the call to Browse().
//
// CREDITS:
// (1) Article by Pete Arends [10/14/2001, www3.telus.net/pja/CFileEditCtrl.htm] -- CFileEditCtrl.  This CEdit-derived 
// class provides a single control for browsing and editing pathnames.  It combines the functionality of CEdit, 
// CFileDialog, and SHBrowseForFolder() in one neat package.  We adapt its usage of SHBrowseForFolder() in CDirChooser. 
//
//
// REVISION HISTORY:
// 07jan2003-- Created. 
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "dirchooser.h" 


#ifdef _DEBUG
#define new DEBUG_NEW                        // version of new operator for detecting memory leaks in debug release. 
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//=== CDirChooser [constructor] ======================================================================================= 
//
//    Upon construction, the currently "chosen" directory defaults to the file system's temporary directory.
// 
CDirChooser::CDirChooser()
{
   LPTSTR lptstr = m_strDir.GetBuffer( _MAX_PATH );            // init current dir to the system temp directory
   ::GetTempPath( _MAX_PATH, lptstr );
   m_strDir.ReleaseBuffer();
   int last = m_strDir.GetLength() - 1;                        // make sure there's no trailing slash.
   if( m_strDir[last] == _T('\\') ) m_strDir.Delete( last );
}


//=== Browse ========================================================================================================== 
//
//    Invoke the Windows shell API SHBrowseForFolder(), allowing user to browse for a directory in the file system 
//    hierarchy. 
//
//    ARGS:       hWndOwner   -- [in] owner window for the browse dialog; if invalid, NULL is used.
//                szTitle     -- [in] text that appears above the browse tree.
//                szInitDir   -- [in] if this specifies the full pathname to a valid file system directory, then it is 
//                               used as the starting point for the browse operation (default = NULL).
// 
//    RETURNS:    TRUE if user selected a directory; FALSE if user cancelled. 
//
BOOL CDirChooser::Browse( HWND hWndOwner, LPCTSTR szTitle, LPCTSTR szInitDir /* =NULL */ )
{
   BROWSEINFO brInfo;                                       // prepare browse info struct for SHBrowseForFolder
   ::ZeroMemory( &brInfo, sizeof(BROWSEINFO) );
   brInfo.hwndOwner = ::IsWindow( hWndOwner ) ? hWndOwner : NULL; 
   brInfo.lpszTitle = szTitle;
   brInfo.ulFlags = BIF_RETURNONLYFSDIRS;                   //    permit access to file system directories only
   brInfo.lpfn = BrowseCallback;                            //    callback used to select current directory once the 
   brInfo.lParam = (LPARAM) this;                           //    browse dialog box is initialized

   CString strSave = m_strDir;                              // save "current" directory in case user cancels

   if( szInitDir != NULL )                                  // if arg specifies a valid file system directory, then 
   {                                                        // make it the "current" directory for this chooser
      CFileFind finder; 
      if( finder.FindFile( szInitDir ) )
      {
         finder.FindNextFile();
         if( finder.IsDirectory() ) m_strDir = szInitDir;
      }
      finder.Close();
   }

   // invoke browse dialog. If user OK'd out of dialog, get path folder selected
   BOOL bRet = FALSE; 
   ITEMIDLIST *idl = static_cast<ITEMIDLIST *>(::SHBrowseForFolder( &brInfo )); 
   if(idl) 
   {
      TCHAR lpstrBuffer[_MAX_PATH] = _T("");                //    get path string from ITEMIDLIST
      if( ::SHGetPathFromIDList( idl, lpstrBuffer ) )
      {
         m_strDir = lpstrBuffer;                            //    remember it!
         int last = m_strDir.GetLength() - 1;               //    get rid of trailing slash, if any
         if( m_strDir[last] == _T('\\') ) 
            m_strDir.Delete( last );
         bRet = TRUE;                                       //    success!
      }

      LPMALLOC lpm;                                         // free memory returned by SHBrowseForFolder
      if( ::SHGetMalloc( &lpm ) == NOERROR )
         lpm->Free(idl); 
   }

   if( !bRet ) m_strDir = strSave;                          // on failure or cancel, restore "current" directory

   return( bRet );
}


//=== BrowseCallback ================================================================================================== 
//
//    This callback function is invoked by ::SHBrowseForFolder() to permit tailoring certain aspects of the browse 
//    dialog's appearance.  Here, we use it to select an initial directory once the dialog has been initialized.
//
//    ARGS:       hWnd     -- [in] the browse dialog's window handle.
//                nMsg     -- [in] the message/event from the browse dialog.  we only respond to BFFM_INITIALIZED.
//                lParam   -- [in] message-specific info.  not used for BFFM_INITIALIZED.
//                lpData   -- [in] application-defined data specified in the lParam field of the BROWSEINFO struct; we 
//                            use it to pass a reference to THIS obj, since the callback must be a static method.
// 
//    RETURNS:    0 always.
//
int CALLBACK CDirChooser::BrowseCallback( HWND hWnd, UINT nMsg, LPARAM lParam, LPARAM lpData )
{
   if( nMsg == BFFM_INITIALIZED )                           // once browse dlg is initialized, we want to select the 
   {                                                        // "current" directory as a starting point:
      CDirChooser* pThis = (CDirChooser*) lpData;           //    for access to nonstatic members!

      CString szPath = pThis->m_strDir;                     //    path to current directory                   
      if( szPath.Left(2) != _T("\\\\") )                    //    SHBrowseForFolder does not like UNC path
      {
         int len = szPath.GetLength() - 1;
         if( len != 2 && szPath[len] == _T('\\') )          //    SHBrowseForFolder does not like trailing slash 
            szPath.Delete(len); 
         ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPCTSTR)szPath);
      }
   }
   return 0;
}

