//===================================================================================================================== 
//
// cxrmvstoredlg.h : Declaration of CCxRMVStoreDlg, a MAESTRO control panel dialog page for managing the contents of
//                   the RMVideo "media store".
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXRMVSTOREDLG_H__)
#define CXRMVSTOREDLG_H__


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cxobj_ifc.h"                    // MAESTRO object interface:  common constants and other defines 
#include "fileeditctrl\fileeditctrl.h"    // for CFileEditCtrl
#include "cxcontrolpaneldlg.h"            // CCxControlPanelDlg -- base class for MAESTRO mode contol panel dialogs



//===================================================================================================================== 
// Declaration of class CCxRMVStoreDlg 
//===================================================================================================================== 
//
class CCxRMVStoreDlg : public CCxControlPanelDlg
{
   DECLARE_DYNCREATE( CCxRMVStoreDlg )

//===================================================================================================================== 
// CONSTANTS
//===================================================================================================================== 
private:
   static const int IDD = IDD_RMVSTORE;         // dialog template resource ID for this dialog


//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private:
   CFileEditCtrl m_fecSrcPath;                           // ctrl selects src path of media file to be downloaded
   CEdit m_edFolderName;                                 // edits dst folder name for a media file download
   CEdit m_edFileName;                                   // edits dst file name for a media file download

   CButton m_btnDelFolder;                               // the "Delete folder" pushbutton
   CButton m_btnDelFile;                                 // the "Delete file" pushbutton
   CButton m_btnDownload;                                // the "Download file" pushbutton

   CListBox m_folderList;                                // list box showing all folders in media store
   CListBox m_fileList;                                  // list box showing all media files in a selected folder

//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxRMVStoreDlg(const CCxRMVStoreDlg& src);            // no copy constructor or assignment operator defined
   CCxRMVStoreDlg& operator=( const CCxRMVStoreDlg& src ); 

public: 
   CCxRMVStoreDlg() : CCxControlPanelDlg( IDD )  {}
   ~CCxRMVStoreDlg() {}


//===================================================================================================================== 
// MESSAGE MAP HANDLERS
//===================================================================================================================== 
protected:
   afx_msg void OnChange(UINT id);                       // handle input to edit ctrls, buttons

   DECLARE_MESSAGE_MAP()

private:
   void DeleteCurrentSelection(BOOL bFolder);            // deletes currently selected folder or single media file
   void OnFolderSelectionChange();                       // invoked when currently selected media folder changes
   void DownloadMediaFile();                             // download media file as specified, if possible
   
//===================================================================================================================== 
// OPERATIONS
//===================================================================================================================== 
public:
   BOOL OnInitDialog();                                  // prepare dialog for display
   VOID Refresh();                                       // to refresh dlg appearance -- updates enable state of ctrls
   VOID Load();                                          // reload all ctrls IAW current RMVideo media store content
};


#endif // !defined(CXRMVSTOREDLG_H__)

