//=====================================================================================================================
//
// cxrmvstoredlg.cpp : Declaration of CCxRMVStoreDlg, a MAESTRO control panel dialog page for managing the contents of
//                     the RMVideo "media store".
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Two RMVideo target classes require media files that are stored on the RMVideo host machine: the RMV_MOVIE target 
// plays a video, while the RMV_IMAGE target displays a static image. All video and image files referenced by these
// target types are maintained in a "media store", a collection of up to RMV_MVF_LIMIT (50) folders, each of which can 
// contain RMV_MVF_LIMIT media files. CCxRMVStoreDlg allows the user to view media store contents, download media files 
// to the RMVideo server, and remove media folders or files. Since some file downloading can take a significant amount 
// of time, CCxRMVStoreDlg is available only in the IdleMode operational state. It should not be available in any other
// op mode.
//
// Unlike other mode control panel dialogs, CCxRMVStoreDlg reflects information that is retrieved from MaestroDriver 
// and is not maintained as application settings -- the "table of contents" for the media store. If MaestroDriver is
// restarted at some point, the contents listed in the dialog could become "stale". Since there's currently no mechanism
// for reloading dialog content upon such a restart, it is expected that the IdleMode controller call Load() upon
// entering IdleMode. This will reload the media store content to ensure it is up-to-date.
//
// ==> Summary of controls housed on the dialog
//
//    IDC_RMV_FOLDERLIST ... IDC_RMV_FILELIST [list box]: List of folders in the RMVideo media store; list of media 
//       files within a selected folder. Both are configured as single-selection lists with the LBS_NOTIFY style set.
//    IDC_RMV_FOLDER ... IDC_RMV_FILE [edit]: Destination folder name and file name for a downloaded media file.
//    IDC_RMV_SRC [custom edit]: This custom edit ctrl displays the full pathname of the media file that is to be 
//       downloaded from the MAESTRO host file system to the RMVideo movie store. It includes a "browse" button which 
//       invokes a dialog that lets the user choose a different path. A standard edit control is subclassed to 
//       CFileEditCtrl to get the browsing functionality. See CFileEditCtrl for details.
//    IDC_RMV_DELFOLDER ... IDC_RMV_DOWNLOAD [push]: Delete the currently selected media folder, delete the currently
//       selected media file, or initiate a media file download.
//
// [**NOTE: Note that the control ID ranges indicated above span a contiguous range of values so that we can use the 
// macro ON_CONTROL_RANGE in the message map.]
//
// ==> The MAESTRO "Mode Control" Framework.
// MAESTRO's master mode control panel CCxControlPanel is implemented as a dockable dialog bar containing one or more
// tabbed dialogs.  All dialogs that affect runtime state in any MAESTRO operational mode are installed in this
// container, although only a subset of them will be accessible in any given mode.  In addition to its role as a
// dialog container, CCxControlPanel constructs a "mode controller" object for each op mode, and it handles mode
// switches by invoking appropriate methods on the relevant mode controllers.  Each mode controller, interacting with
// the operator via some subset of the mode control panel dialogs, encapsulates the runtime behavior of MAESTRO and
// MAESTRODRIVER in a particular operational mode.  To communicate with MAESTRODRIVER, it must invoke methods on the
// MAESTRO runtime interface, CCxRuntime.  By design, the mode controller should insulate the mode control dialogs from
// CCxRuntime.  In other words, it provides all the methods needed by the dialogs to realize the desired functionality
// of the operational mode that controller and the associated dialogs represent.  Multiple dialogs allow us to break up
// that functionality into logically grouped, more manageable chunks.
//
// We define two ABSTRACT classes that serve as the foundations for this "mode control" framework.  CCxModeControl is
// the base class for all MAESTRO mode controller objects, while CCxControlPanelDlg is the base class for any dialog
// that is installed in CCxControlPanel.  CCxModeControl handles tasks that are common to all mode controllers and
// defines a set of abstract methods that any realizable mode controller must implement; CCxControlPanelDlg does the
// same for mode control dialog objects.
//
// See the implementation files for CCxControlPanel, CCxControlPanelDlg, and CCxModeControl for more details.
//
//
// CREDITS:
// (1) CCxRMVStoreDlg is ultimately built upon the CSizingTabDlgBar/CSzDlgBarDlg framework which, in turn, is based on
// the MFC extension CSizingControlBarCF by Cristi Posea.  See szdlgbar.cpp for credits.
//
// REVISION HISTORY:
// 04sep2009-- Began development.
// 11oct2016-- Revised IAW a generalization of the notion of RMVideo's "movie store" as a "media store", since it can
//             now contain both video files for the RMV_MOVIE target and image files for the RMV_IMAGE target. Some
//             control ID and member variable names changed; and implementation updated IAW same-dated changes to
//             CCxModeControl.
//=====================================================================================================================


#include "stdafx.h"                          // standard MFC stuff
#include "cntrlx.h"                          // CCntrlxApp and resource IDs for MAESTRO

#include "cxmodecontrol.h"                   // CCxModeControl -- base class for MAESTRO mode controllers
#include "util.h"

#include "cxrmvstoredlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE( CCxRMVStoreDlg, CCxControlPanelDlg )

BEGIN_MESSAGE_MAP( CCxRMVStoreDlg, CCxControlPanelDlg )
   ON_CONTROL_RANGE( LBN_SELCHANGE, IDC_RMV_FOLDERLIST, IDC_RMV_FILELIST, OnChange )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_RMV_DELFOLDER, IDC_RMV_DOWNLOAD, OnChange )
END_MESSAGE_MAP()


//=====================================================================================================================
// MESSAGE MAP HANDLERS
//=====================================================================================================================

/**
 * Respond to various notifications from selected child controls on the dialog:
 *    1) BN_CLICKED ==> User clicked one of the pushbuttons. The appropiate action is taken, if possible: delete the
 * selected media folder, delete the selected media file, or download a file to the RMVideo media store. These are
 * relatively slow operations, so the wait cursor is displayed and the application window disabled.
 *    2) LBN_SELCHANGE ==> When the selection changes within the media folder list box, we repopulate the media file 
 * list box accordingly and place the selected folder's name in the destination folder name text field. When the 
 * selection changes within either list box, we update the enable state of the relevant "Delete" button.
 *
 * @param id Resource ID of the child control that sent the notification.
 */
void CCxRMVStoreDlg::OnChange(UINT id)
{
   if(id == IDC_RMV_DELFOLDER || id == IDC_RMV_DELFILE)
      DeleteCurrentSelection(id == IDC_RMV_DELFOLDER);
   else if(id == IDC_RMV_DOWNLOAD)
      DownloadMediaFile();
   else if(id == IDC_RMV_FOLDERLIST || id == IDC_RMV_FILELIST)
   {
      if(id == IDC_RMV_FOLDERLIST) OnFolderSelectionChange();
      Refresh();
   }
}

/**
 * Delete the currently selected RMVideo media folder or media file. If the operation is successful, the list boxes 
 * that expose the media store content are updated appropriately: (1) If a media file was deleted and it was the last
 * one in a folder, that folder is also removed. (2) When a folder is removed, the folder list is repopulated IAW the
 * folder that is selected (if any are left) after the removal. If the folder selection changes, the "Folder" field
 * in the "Download" section is set to the name of the selected folder.
 *
 * A wait cursor is displayed and the application mainframe is disabled because the delete operation may take up to 
 * five seconds to complete.
 *
 * @param bFolder If TRUE, delete the currently selected folder; else, delete the currently selected movie.
 */
void CCxRMVStoreDlg::DeleteCurrentSelection(BOOL bFolder)
{
   int iFolder = m_folderList.GetCurSel();
   if(iFolder == LB_ERR) { Refresh(); return; }
   int iFile = -1;
   if(!bFolder)
   {
      iFile = m_fileList.GetCurSel();
      if(iFile == LB_ERR) { Refresh(); return; }
   }
   
   // the delete operation may take a little while...
   CWaitCursor wait;
   AfxGetMainWnd()->EnableWindow(FALSE);
   BOOL bOk = GetCurrentModeCtrl()->DeleteRMVMediaFile(iFolder, iFile);
   AfxGetMainWnd()->EnableWindow(TRUE);
   
   // if successful, update our list box contents accordingly
   if(bOk)
   {
      BOOL bDelFolder = (iFile == -1);
      if(iFile != -1)
      {
         m_fileList.SetCurSel(-1);
         int n = m_fileList.DeleteString(iFile);
         if(n > 0) m_fileList.SetCurSel((iFile < n) ? iFile : n-1);
         else bDelFolder = TRUE;
      }
      if(bDelFolder)
      {
         m_folderList.SetCurSel(-1);
         int n = m_folderList.DeleteString(iFolder);
         if(n > 0) m_folderList.SetCurSel((iFolder < n) ? iFolder : n-1);
         OnFolderSelectionChange();
      }
      Refresh();
   }
}

/**
 * Whenever the current selection changes in the media folder list, the file list is emptied and then repopulated with
 * entries describing the media files in the newly selected folder.
 */
void CCxRMVStoreDlg::OnFolderSelectionChange()
{
   CCxModeControl* pCtrl = GetCurrentModeCtrl();
   CString folderName = "";
   int iFolder = m_folderList.GetCurSel();
   if(iFolder != LB_ERR) pCtrl->GetRMVMediaFolder(iFolder, folderName);
   
   m_fileList.SetRedraw(FALSE);
   m_fileList.SetCurSel(-1);
   m_fileList.ResetContent();
   int nFiles = (iFolder != LB_ERR) ? pCtrl->GetNumRMVMediaFiles(iFolder) : 0;
   for(int i=0; i<nFiles; i++)
   {
      CString mediaName;
      CString mediaDesc;
      CString itemStr;
      pCtrl->GetRMVMediaInfo(iFolder, i, mediaName, mediaDesc);
      itemStr.Format("%s  [%s]", mediaName, mediaDesc);
      m_fileList.AddString(itemStr);
   }
   if(nFiles > 0) m_fileList.SetCurSel(0);
   m_fileList.SetRedraw(TRUE);
   m_fileList.Invalidate();
   m_fileList.UpdateWindow();
   
   m_edFolderName.SetWindowText(folderName);
}

/**
 * Download a video or image file to the RMVideo media store, using the file pathname, destination folder and file 
 * name specified in the relevant dialog widgets. The method will initiate the download only if the specified file 
 * exists; the destination media folder and file names contain only characters from RMV_MVF_CHARS and are no more 
 * than RMV_MVF_LEN characters in length; the destination does not already exist; and the media store capacity has not 
 * been reached (n more than RMV_MVF_LIMIT folders in the store, and no more than RMV_MVF_LIMIT files per folder).
 *
 * If the operation is not possible, a popup message dialog informs the user. Otherwise, the wait cursor is displayed
 * and the application frame window is disabled during the download, which can take an indefinite time to finish.
 */
void CCxRMVStoreDlg::DownloadMediaFile()
{
   CString str;
   CCxModeControl* pCtrl = GetCurrentModeCtrl();
   
   // verify that source path identifies an existing file.
   CString strPath;
   m_fecSrcPath.GetWindowText(strPath);
   CFileStatus fileStatus;
   if(!CFile::GetStatus(strPath, fileStatus))
   {
      str.Format("Media file [%s] not found!", strPath);
      ::AfxMessageBox(str, MB_OK|MB_ICONEXCLAMATION);
      return;
   }
   
   // verify that media folder and file names meet RMVideo restrictions
   CString folder;
   m_edFolderName.GetWindowText(folder);
   int len = folder.GetLength();
   str = folder.SpanIncluding((LPCTSTR)RMV_MVF_CHARS);
   if(len == 0 || len >= RMV_MVF_LEN || len != str.GetLength())
   {
      str.Format("Destination media folder name [%s] is invalid!", folder);
      ::AfxMessageBox(str, MB_OK|MB_ICONEXCLAMATION);
      return;
   }
   CString fName;
   m_edFileName.GetWindowText(fName);
   len = fName.GetLength();
   str = fName.SpanIncluding((LPCTSTR)RMV_MVF_CHARS);
   if(len == 0 || len >= RMV_MVF_LEN || len != str.GetLength())
   {
      str.Format("Destination media file name [%s] is invalid!", fName);
      ::AfxMessageBox(str, MB_OK|MB_ICONEXCLAMATION);
      return;
   }

   // if destination folder exists, make sure it is not full and that the destination file name is not already in use; 
   // if destination folder does not exist, make sure media store is not full
   int iFolder = m_folderList.FindString(0, folder);
   if(iFolder != LB_ERR)
   {
      int nFiles = pCtrl->GetNumRMVMediaFiles(iFolder);
      if(nFiles == RMV_MVF_LIMIT)
      {
         str.Format("Destination media folder [%s] is full!", folder);
         ::AfxMessageBox(str, MB_OK|MB_ICONEXCLAMATION);
         return;
      }
      for(int i=0; i<nFiles; i++)
      {
         CString name;
         CString desc;
         pCtrl->GetRMVMediaInfo(iFolder, i, name, desc);
         if(fName.Compare(name) == 0)
         {
            str.Format("Media file destination [%s/%s] already exists in RMVideo media store!", folder, fName);
            ::AfxMessageBox(str, MB_OK|MB_ICONEXCLAMATION);
            return;
         }
      }
   }
   else
   {
      iFolder = -1;
      if(pCtrl->GetNumRMVMediaFolders() == RMV_MVF_LIMIT)
      {
         ::AfxMessageBox("RMVideo media store already has the maximum number of folders!", MB_OK|MB_ICONEXCLAMATION);
         return;
      }
   }

   // perform the download
   CWaitCursor wait;
   AfxGetMainWnd()->EnableWindow(FALSE);
   BOOL bOk = pCtrl->DownloadRMVMediaFile(strPath, iFolder, folder, fName);
   AfxGetMainWnd()->EnableWindow(TRUE);
   
   // display message box informing user of success/failure
   if(bOk) ::AfxMessageBox("Media file download successful.", MB_OK|MB_ICONINFORMATION);
   else ::AfxMessageBox("Download failed. Check message log for details.", MB_OK|MB_ICONEXCLAMATION);
   
   // if successful, update our list box contents accordingly
   if(bOk)
   {
      if(iFolder != -1)
      {
         if(m_folderList.GetCurSel() != iFolder) m_folderList.SetCurSel(iFolder);
         OnFolderSelectionChange();
         m_fileList.SetCurSel(pCtrl->GetNumRMVMediaFiles(iFolder) - 1);
      }
      else
      {
         m_folderList.SetCurSel(-1);
         int idx = pCtrl->GetNumRMVMediaFolders() - 1;
         m_folderList.AddString(folder);
         m_folderList.SetCurSel(idx);
         OnFolderSelectionChange();
      }
      Refresh();
   }
}


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

/**
 * [base class override] Prepare the dialog for display. Here we subclass dlg resource template-defined controls to 
 * class members and initialize all to "start-up" conditions. Everything is empty initially because we may not have 
 * access to RMVideo media store contents when this method is called. 
 * @return TRUE to place initial input focus on the first ctrl in dialog's tab order; FALSE if we've already set the 
 * input focus on another ctrl.
 */
BOOL CCxRMVStoreDlg::OnInitDialog()
{
   CCxControlPanelDlg::OnInitDialog();                                     // let base class do its thing...

   m_fecSrcPath.SubclassDlgItem(IDC_RMV_SRC, (CWnd*)this );
   m_fecSrcPath.SetFlags(FEC_FILE|FEC_BUTTONTIP);
   m_fecSrcPath.SetWindowText("");
   
   m_edFolderName.SubclassDlgItem(IDC_RMV_FOLDER, (CWnd*) this );
   m_edFolderName.SetLimitText(RMV_MVF_LEN);
   m_edFolderName.SetWindowText("");
   
   m_edFileName.SubclassDlgItem(IDC_RMV_FILE, (CWnd*) this );
   m_edFileName.SetLimitText(RMV_MVF_LEN);
   m_edFileName.SetWindowText("");
   
   m_btnDelFolder.SubclassDlgItem(IDC_RMV_DELFOLDER, (CWnd*) this);
   m_btnDelFile.SubclassDlgItem(IDC_RMV_DELFILE, (CWnd*) this);
   m_btnDownload.SubclassDlgItem(IDC_RMV_DOWNLOAD, (CWnd*) this);
   
   m_folderList.SubclassDlgItem(IDC_RMV_FOLDERLIST, (CWnd*) this);
   m_fileList.SubclassDlgItem(IDC_RMV_FILELIST, (CWnd*) this);

   return(TRUE);
}

/**
 * This method updates the enable state of the pushbuttons on the dialog. The "Download" button is enabled as long as
 * the op mode is IdleMode and RMVideo is available. The "Delete Folder" and "Delete File" buttons are also enabled in
 * this case, unless there's no folder or media file selected from the corresponding list.
 */
VOID CCxRMVStoreDlg::Refresh()
{
   BOOL bEnabled = GetCurrentModeCtrl()->CanUpdateRMV();
   m_btnDownload.EnableWindow(bEnabled);
   m_btnDelFolder.EnableWindow(bEnabled && (m_folderList.GetCurSel() != LB_ERR));
   m_btnDelFile.EnableWindow(bEnabled && (m_fileList.GetCurSel() != LB_ERR));
}

/**
 * This method loads (or reloads) the folder and file lists to reflect the current contents of the RMVideo media store.
 * 
 * DEVNOTE: In the event that MaestroDriver is restarted, it is possible that the media store contents will have changed
 * or that RMVideo will no longer be available. Since there's currently no mechanism to reload control panel dialogs in 
 * the event of such a restart, The IdleMode controller should call this method upon entering IdleMode to ensure the 
 * dialog's representation of media store contents is up-to-date. 
 */
VOID CCxRMVStoreDlg::Load()
{
   CCxModeControl* pCtrl = GetCurrentModeCtrl();
   
   m_folderList.SetRedraw(FALSE);
   m_folderList.SetCurSel(-1);
   m_folderList.ResetContent();
   int nFolders = pCtrl->GetNumRMVMediaFolders();
   for(int i=0; i<nFolders; i++)
   {
      CString folder;
      pCtrl->GetRMVMediaFolder(i, folder);
      m_folderList.AddString(folder);
   }
   if(nFolders > 0) m_folderList.SetCurSel(0);
   m_folderList.SetRedraw(TRUE);
   m_folderList.Invalidate();
   m_folderList.UpdateWindow();
   
   OnFolderSelectionChange();    // this will load the media file list
}
