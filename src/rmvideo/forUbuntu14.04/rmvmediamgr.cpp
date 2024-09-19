//=====================================================================================================================
//
// rmvmediamgr.cpp : Implementation of class CRMVMediaMgr, which manages RMVideo's "media store".
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// RMVideo stores video and image files on the host system which, in turn, serve as the source files for the RMV_MOVIE
// and RMV_IMAGE target classes. CRMVMediaMgr is the singleton object that manages RMVideo's media file storage and
// handles Maestro requests for information about the current contents of that store.
//
// All media files are maintained in folders within the "media" folder in the RMVideo installation directory (which is
// always assumed to be the current working directory!). Thus, each media file -- whether video or image -- is
// "identified" by its filename and the name of its parent folder. No media files are stored in the "media" folder
// itself. During startup, CRMVMediaMgr::load() must be called; this method scans the entire media store and prepares a
// "table of contents" summarizing information on all recognized media files available. CRMVMediaMgr maintains the TOC
// internally so that it can respond to Maestro requests for media store information, update the TOC as media files are
// added or removed, etc.
//
// The RMV_IMAGE target was introduced in RMVideo version 7. Prior to this change, only video files were stored in what
// was known as the the "movie store". With the introduction of the RMV_IMAGE target came the need to store image files
// as well, so the "movie store" was generalized as a "media store". CRMVMediaMgr extends and replaces the its
// predecessor, CRMVMovieMgr.
//
// CREDITS: We use the single-file header-style library STB_IMAGE.H from Sean T. Barrett to load image data from JPEG,
// PNG, BMP, PSD, and GIF files. The library does not support all the nuances of these image file formats -- see the
// header file for details. Also see: https://github.com/nothings/stb/blob/master/stb_image.h.
//
// REVISION HISTORY:
// 23jul2009-- Began development of CRMVMovieMgr.
// 03oct2016-- Generalizing "movie store" as a "media store" for both video and image files, to support a new RMVideo
// target class that presents a static image read from a JPEG, PNG, BMP, PSD, GIF, or TGA file. Using the single-file
// library STB_IMAGE for loading image data from one of the supported files. CRMVMovieMgr renamed as CRMVMediaMgr. 
// Image file information and image loading routines implemented here.
// 24apr2019-- Got rid of the individual CRMVTarget implementations. CRMVTarget is now a concrete class that handles
// all RMVideo target types, including RMV_MOVIE. Video support functions like getVideoInfo() now in CRMVTarget.
// 12aug2019-- Added an internal image cache to CRMVMediaMgr, to address the fact that large images (eg, 2560x1440) 
// take a relatively long time to load from file into memory. The image cache is preloaded when the media store is
// loaded at startup. Cache capacity is about 300MB. Max image dimension is now 5120 pixels.
// 18sep2019-- Refactor: CRMVTarget::getVideoInfo() moved to new class CVidBuffer.
//=====================================================================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

// this is a single-file header-style library providing support for loading basic image files in select formats. Here
// I tailor it to include code supporting JPG, PNG, BMP, PSD, and GIF formats. It only supports the most common
// versions of these image file formats.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_PSD
#define STBI_ONLY_GIF
#include "stb_image.h"

#include "vidbuffer.h"         // for static helper function CVidBuffer::getVideoInfo()
#include "rmvmediamgr.h"

const char* CRMVMediaMgr::MEDIASTOREDIR = "media";
const char* CRMVMediaMgr::OLDSTOREDIR = "movies";
const unsigned long CRMVMediaMgr::MAX_IMGCACHESZ = 300000000L;
const unsigned long CRMVMediaMgr::MAX_IMAGEDIM = 5120;

/**
 Construct the RMVideo media store manager, initially empty. Call load() to scan the dedicated media directory and
 prepare the store's current "table of contents".
*/
CRMVMediaMgr::CRMVMediaMgr()
{ 
   m_bLoaded = false;
   m_nMediaFolders = 0;
   m_pFirstMediaFolder = NULL;
   m_pReplyBuf = NULL;
   m_nReplyBufSz = 0;

   m_nCachedImages = 0;
   m_nCacheSize = 0L;
   m_pImageCache = NULL;
}

/**
 Destroy the RMVideo media store manager, releasing all memory allocated for its internal representation of the media
 store's table of contents, as well as the image cache.
*/
CRMVMediaMgr::~CRMVMediaMgr()
{
   if(m_pReplyBuf != NULL)
   {
      ::free(m_pReplyBuf);
      m_pReplyBuf = NULL;
      m_nReplyBufSz = 0;
   }
   
   while(m_pFirstMediaFolder != NULL)
   {
      MediaFolder* pFolder = m_pFirstMediaFolder;
      m_pFirstMediaFolder = pFolder->pNext;
      while(pFolder->pFirstMedia != NULL)
      {
         MediaInfo* pInfo = pFolder->pFirstMedia;
         pFolder->pFirstMedia = pInfo->pNext;
         ::free(pInfo);
      }
      ::free(pFolder); 
   }
   m_nMediaFolders = 0;
   
   releaseImageCache();

   m_bLoaded = false;
}

/**
 Scan the RMVideo media store directory tree and prepare an internal "table of contents" summarizing information on 
 all valid and supported media files found in the store.
 
 The media store must be located in the subdirectory MEDIASTOREDIR under the current working directory (which should
 be the RMVideo installation directory). If this store directory does not exist, then it is created and the store is
 considered empty. Otherwise, the method examines each immediate subdirectory in the  directory. 

 NOTE: If the pre-version 7 directory OLDSTOREDIR exists and MEDIASTOREDIR does not, the directory is renamed and its 
 subdirectories scanned in the same way. This is how RMVideo converts from the old video-only store to a "media" store
 that can contain both video and image files.

 Scanning the subfolders of MEDIASTOREDIR: If the folder name meets requirments (ASCII alphanumeric characters, plus 
 '.' or '_'; no more than RMV_MVF_LEN characters), then it is added to the table of contents as a media folder. For 
 each file found within a media folder, the file is added to the TOC as a media file if it is a valid video or image
 file supported in RMVideo: For both video and image files, the file name must satisfy the same requirements as for a 
 folder. A video file must: use a format and codec supported by the FFMPEG library, and it must contain a video stream.
 An image file must use one of 5 supported formats (JPEG, PNG, BMP, PSD, or GIF) and must be recognized as such by the
 STBI_IMAGE library. Some information is extracted from the media file for inclusion in the table of contents (file 
 name, image or video frame size; frame rate and total # of frames for a video file).

 To keep the replies to media store queries manageable, there is a limit of RMV_MVF_LIMIT folders in the media store
 and RMV_MVF_LIMIT media files per folder. This method will stop scanning the contents of a particular folder once it
 has found RMV_MVF_LIMIT files, and it will stop scanning altogether once it has found RMV_MVF_LIMIT media folders!

 As the media store is scanned and the TOC constructed, an internal image cache is also preloaded with any images
 found in the store -- until the cache's maximum capacity is reached. If the store contains only a few small images,
 it is likely that all images will be cached. The cache is also updated as new images are uploaded from Maestro, or
 existing images deleted. This internal image cache exists to speed up loading of RMV_IMAGE targets, since reading a 
 large image in from disk can take a significant amount of time.

 All other CRMVMediaMgr methods fail until this method is called successfully. Since it may open a fair number of
 files, it will take an indeterminate amount of time to execute. Invoke only during RMVideo startup.
 
 @return True if successful, false otherwise. Error message printed to stderr.
*/
bool CRMVMediaMgr::load()
{
   if(m_bLoaded) return(true);
   
   fprintf(stderr, "(CRMVMediaMgr) Scanning media store and preparing media file index. PLEASE WAIT...\n");
   struct stat statInfo;
   
   // rename the old "movie store" directory so it becomes the new "media store". Fail if unable to rename.
   if((0 == ::stat(OLDSTOREDIR, &statInfo)) && S_ISDIR(statInfo.st_mode))
   {
      if(0 != ::rename(OLDSTOREDIR, MEDIASTOREDIR))
      {
         fprintf(stderr, "(CRMVMediaMgr) Unable to rename old movie store directory as %s\n", MEDIASTOREDIR);
         return(false);
      }
   }
   
   // if media store directory does not exist try to create it. If it exists but is actually a file, fail.
   if(0 == ::stat(MEDIASTOREDIR, &statInfo))
   {
      if(!S_ISDIR(statInfo.st_mode))
      {
         fprintf(stderr, "(CRMVMediaMgr) Media store directory is a regular file!\n");
         return(false);
      }
   }
   else if(errno == ENOENT)
   {
      if(0 != ::mkdir(MEDIASTOREDIR, 0777))
      {
         perror("(CRMVMediaMgr) Unable to create media store directory!\n");
         return(false);
      }
      else
      {
         fprintf(stderr, "(CRMVMediaMgr) Media store directory created. Store is empty.\n");
         m_bLoaded = true;
         return(true);
      }
   }
   else
   {
      fprintf(stderr, "(CRMVMediaMgr) Unable to verify existence of media store directory!\n");
      return(false);
   }
   
   // scan all directories in the media store directory. Append an empty media "folder" to our TOC corresponding to
   // each directory whose name is RMV_MVF_LEN characters long or less and includes only allowed ASCII characters. Of 
   // course, do not admit the "." and ".." directory entries! Once we find RMV_MVF_LIMIT folders, stop!
   DIR* pStoreDir = ::opendir(MEDIASTOREDIR);
   struct dirent* pDirEntry = NULL;
   if(pStoreDir == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr) Unable to scan media store directory!\n");
      return(false);
   }
   
   char dirPath[100];
   pDirEntry = ::readdir(pStoreDir);
   while(pDirEntry != NULL)
   {
      bool append = (0 != ::strcmp(pDirEntry->d_name, ".")) && (0 != ::strcmp(pDirEntry->d_name, ".."));
      if(append) 
      {
         int len = ::strlen(pDirEntry->d_name);
         append = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(pDirEntry->d_name, RMV_MVF_CHARS));
      }
      if(append)
      {
         ::sprintf(dirPath, "%s/%s", MEDIASTOREDIR, pDirEntry->d_name);
         append = (0 == ::stat(dirPath, &statInfo));
         if(append) append = S_ISDIR(statInfo.st_mode);
      }
      if(append)
      {
         if(!appendMediaFolder(pDirEntry->d_name))
         {
            ::closedir(pStoreDir);
            return(false);
         }
      }
      
      // proceed to next directory, unless we've reached capacity
      pDirEntry = ::readdir(pStoreDir);
      if(pDirEntry != NULL && m_nMediaFolders == RMV_MVF_LIMIT)
      {
         fprintf(stderr, "(CRMVMediaMgr) Max number of media folders reached!\n");
         break;
      }
   }
   ::closedir(pStoreDir);
   
   // scan each media folder and append all valid and supported media files therein. Images are preloaded into an
   // internal image cache to optimize access to large (fullscreen) images. THIS COULD TAKE A WHILE if the media
   // store contains many large images.
   int nTotalMediaFiles = 0;
   MediaFolder* pFolder = m_pFirstMediaFolder;
   while(pFolder != NULL)
   {
      if(!scanMediaFolder(pFolder)) return(false);
      nTotalMediaFiles += pFolder->nMediaFiles;
      pFolder = pFolder->pNext;
   }
   
   fprintf(stderr, "(CRMVMediaMgr) Found %d media files in %d folders; %d images cached.\n", 
      nTotalMediaFiles, m_nMediaFolders, m_nCachedImages);

   m_bLoaded = true;
   return(true);
}

/**
 Reply to the Maestro command RMV_CMD_GETMEDIADIRS, sending back the current list of folders in the media store IAW
 the expected format for the reply payload. See RMVIDEO_COMMON.H. It is assumed that the RMV_CMD_GETMEDIADIRS command
 was just received.

 @param pIOLink The Maestro-RMVideo comm link. Reply is sent over this link.
*/
void CRMVMediaMgr::replyGetMediaDirs(CRMVIo* pIOLink)
{
   if(!m_bLoaded) 
   {
      ::fprintf(stderr, "(CRMVMediaMgr::replyGetMediaDirs) Media store not initialized!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // compute length of string needed to hold list of folder names with intervening nulls. Pad string to ensure its 
   // length is a multiple of 4 bytes so it fits in the int buffer.
   int strLen = 0;
   int nPad = 0;
   MediaFolder* pFolder = m_pFirstMediaFolder;
   while(pFolder != NULL)
   {
      strLen += ::strlen(pFolder->name);
      pFolder = pFolder->pNext;
   }
   strLen += m_nMediaFolders;
   while((strLen % 4) != 0) {++strLen; ++nPad; }
   
   // if our reply buffer is not large enough, free it and allocate a larger one
   int bufSz = 2 + strLen/4;
   if(!ensureSufficientReplyBufSize(bufSz))
   {
      perror("(CRMVMediaMgr::replyGetMediaDirs) Memory allocation failed!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // compose reply. Note how we pack the string bytes into the int buffer.
   m_pReplyBuf[0] = RMV_SIG_CMDACK;
   m_pReplyBuf[1] = m_nMediaFolders;
   if(m_nMediaFolders > 0)
   {
      char* pcBuf = (char *) m_pReplyBuf;
      pcBuf += 8;
      MediaFolder* pFolder = m_pFirstMediaFolder;
      while(pFolder != NULL)
      {
         ::strcpy(pcBuf, pFolder->name);
         pcBuf +=  ::strlen(pFolder->name) + 1;  // move to character after the term null!!
         pFolder = pFolder->pNext;
      }

      for(int i=0; i<nPad; i++)
      {
         *pcBuf = '\0';
         ++pcBuf;
      }
   }

   // send the reply
   pIOLink->sendData(bufSz, m_pReplyBuf);
}

/**
 Reply to the Maestro command RMV_CMD_GETMEDIAFILES, sending back the current list of media files in a specified 
 folder within the media store IAW the expected format for the reply payload. See RMVIDEO_COMMON.H. It is assumed that
 the RMV_CMD_GETMEDIAFILES command was just received, and the name of the media folder for which a file listing was 
 requested should be available from CRMVIo::getMediaFolder()

 @param pIOLink The Maestro-RMVideo comm link. Reply is sent over this link.
*/
void CRMVMediaMgr::replyGetMediaFiles(CRMVIo* pIOLink)
{
   const char* dirName = pIOLink->getMediaFolder();
   
   // find the media folder specified. If not found in our TOC, fail.
   if(!m_bLoaded) 
   {
      fprintf(stderr, "(CRMVMediaMgr::replyGetMediaFiles) Media store not initialized!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   MediaFolder* pFolder = m_pFirstMediaFolder;
   while(pFolder != NULL)
   {
      if(::strcmp(dirName, pFolder->name) == 0)
         break;
      pFolder = pFolder->pNext;
   }
   if(pFolder == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr::replyGetMediaFiles) Media folder '%s' not found!\n", dirName);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // compute length of string needed to hold list of media file names with intervening nulls. Pad string to ensure its
   // length is a multiple of 4 bytes so it fits in the int buffer.
   int strLen = 0;
   int nPad = 0;
   MediaInfo* pInfo = pFolder->pFirstMedia;
   while(pInfo != NULL)
   {
      strLen += ::strlen(pInfo->filename);
      pInfo = pInfo->pNext;
   }
   strLen += pFolder->nMediaFiles;
   while((strLen % 4) != 0) {++strLen; ++nPad; }
   
   // if our reply buffer is not large enough, free it and allocate a larger one
   int bufSz = 2 + strLen/4;
   if(!ensureSufficientReplyBufSize(bufSz))
   {
      perror("(CRMVMediaMgr::replyGetMediaFiles) Memory allocation failed!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // compose reply. Note how we pack the string bytes into the int buffer.
   m_pReplyBuf[0] = RMV_SIG_CMDACK;
   m_pReplyBuf[1] = pFolder->nMediaFiles;
   if(pFolder->nMediaFiles > 0)
   {
      char* pcBuf = (char *) m_pReplyBuf;
      pcBuf += 8;
      MediaInfo* pInfo = pFolder->pFirstMedia;
      while(pInfo != NULL)
      {
         ::strcpy(pcBuf, pInfo->filename);
         pcBuf +=  ::strlen(pInfo->filename) + 1;   // move past the term null!!
         pInfo = pInfo->pNext;
      }

      for(int i=0; i<nPad; i++)
      {
         *pcBuf = '\0';
         ++pcBuf;
      }
   }

   // send the reply
   pIOLink->sendData(bufSz, m_pReplyBuf);
}

/**
 Reply to the Maestro command RMV_CMD_GETMEDIAINFO, sending back information on the specified media file within the 
 media store IAW the expected format for the reply payload. See RMVIDEO_COMMON.H. It is assumed that the 
 RMV_CMD_GETMEDIAINFO command was just received, and the names of the media file and its parent folder should be 
 available from CRMVIo::getMediaFile(), getMediaFolder();

 @param pIOLink The Maestro-RMVideo comm link. Reply is sent over this link.
*/
void CRMVMediaMgr::replyGetMediaInfo(CRMVIo* pIOLink)
{
   const char* dirName = pIOLink->getMediaFolder();
   const char* fName = pIOLink->getMediaFile();
   
   // find the info on the media file specified. If not found in our TOC, fail.
   if(!m_bLoaded) 
   {
      fprintf(stderr, "(CRMVMediaMgr::replyGetMediaInfo) Media store not initialized!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   MediaFolder* pFolder = m_pFirstMediaFolder;
   while(pFolder != NULL)
   {
      if(::strcmp(dirName, pFolder->name) == 0) break;
      pFolder = pFolder->pNext;
   }
   if(pFolder == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr::replyGetMediaInfo) Media folder '%s' not found!\n", dirName);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }

   MediaInfo* pInfo = pFolder->pFirstMedia;
   while(pInfo != NULL)
   {
      if(::strcmp(fName, pInfo->filename) == 0) break;
      pInfo = pInfo->pNext;
   }
   if(pInfo == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr::replyGetMediaInfo) Media file '%s/%s' not found!\n", dirName, fName);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // if our reply buffer is not large enough, free it and allocate a larger one
   if(!ensureSufficientReplyBufSize(5))
   {
      perror("(CRMVMediaMgr::replyGetMediaInfo) Memory allocation failed!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }

   // compose and send the reply
   m_pReplyBuf[0] = RMV_SIG_CMDACK;
   m_pReplyBuf[1] = pInfo->width;
   m_pReplyBuf[2] = pInfo->height;
   m_pReplyBuf[3] = pInfo->isVideo ? pInfo->rate : -1;
   m_pReplyBuf[4] = pInfo->isVideo ? pInfo->dur : -1;
   pIOLink->sendData(5, m_pReplyBuf);
}

/**
 Reply to the Maestro command RMV_CMD_DELETEMEDIA, permanently deleting the specified media file or an entire media
 folder from the media store -- sending RMV_SIG_CMDACK to confirm the operation or RMV_SIG_CMDERR to indicate the
 operation failed. It is assumed that the RMV_CMD_DELETEMMEDIA command was just received, and the names of the media
 file and its parent folder should be available from CRMVIo::getMediaFile() and getMediaFolder(), respectively. When 
 removing an entire media folder, the media file name will be zero-length.

 @param pIOLink The Maestro-RMVideo comm link. Reply is sent over this link.
*/
void CRMVMediaMgr::replyDeleteMediaFile(CRMVIo* pIOLink)
{
   const char* dirName = pIOLink->getMediaFolder();
   const char* fName = pIOLink->getMediaFile();
   
   // find the info on the movie file specified. If not found in our TOC, fail.
   if(!m_bLoaded) 
   {
      ::fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) Media store not initialized!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   MediaFolder* pFolder = m_pFirstMediaFolder;
   MediaFolder* pPrevFolder = NULL;
   while(pFolder != NULL)
   {
      if(::strcmp(dirName, pFolder->name) == 0) break;
      pPrevFolder = pFolder;
      pFolder = pFolder->pNext;
   }
   if(pFolder == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) Media folder '%s' not found!\n", dirName);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }

   // if media file specified, then delete just that file. If it is the last media file, remove the folder as well.
   // Otherwise, remove the media folder specified, including all media files therein.
   char path[256];
   if(::strlen(fName) > 0)
   {
      MediaInfo* pInfo = pFolder->pFirstMedia;
      MediaInfo* pBef = NULL;
      while(pInfo != NULL)
      {
         if(::strcmp(fName, pInfo->filename) == 0) break;
         pBef = pInfo;
         pInfo = pInfo->pNext;
      }
      if(pInfo == NULL)
      {
         ::fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) Media file '%s/%s' not found!\n", dirName, fName);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }

      // remove the file. Note that path is relative to the RMVideo installation directory.
      ::sprintf(path, "%s/%s/%s", MEDIASTOREDIR, dirName, fName);
      if(::remove(path) != 0)
      {
         ::fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) Unable to delete file: %s\n", path);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
   
      // if removed file was an image file, make sure the image has been removed from internal image cache
      if(!pInfo->isVideo) removeImageFromCache(dirName, fName);
     
      // remove entry for file from the TOC
      if(pBef != NULL) pBef->pNext = pInfo->pNext;
      else pFolder->pFirstMedia = pInfo->pNext;
      ::free(pInfo);
      --(pFolder->nMediaFiles);
      
      // if media folder is now empty, try to remove corresponding file system directory. If this fails, the directory
      // may contain some non-media files. Leave the empty folder in our TOC in this case; else remove it. In either
      // case, report success, since we did remove the movie file.
      if(pFolder->nMediaFiles == 0)
      {
         ::sprintf(path, "%s/%s", MEDIASTOREDIR, dirName);
         if(0 != ::rmdir(path))
         {
            fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) WARNING: Unable to remove directory corres to\n");
            fprintf(stderr, "   now-empty media folder '%s'. Directory may contain other non-media files\n", path);
         }
         else
         {
            if(pPrevFolder == NULL) m_pFirstMediaFolder = pFolder->pNext;
            else pPrevFolder->pNext = pFolder->pNext;
            --m_nMediaFolders;
            ::free(pFolder);
         }
      }
   }
   else
   {
      // remove all the files in the folder
      while(pFolder->pFirstMedia != NULL)
      {
         MediaInfo* pInfo = pFolder->pFirstMedia;
         
         // remove the file. Note that path is relative to the RMVideo installation directory.
         ::sprintf(path, "%s/%s/%s", MEDIASTOREDIR, dirName, pInfo->filename);
         if(::remove(path) != 0)
         {
            fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) Unable to remove file '%s'\n", path);
            pIOLink->sendSignal(RMV_SIG_CMDERR);
            return;
         }
         
         // if removed file was an image file, make sure the image has been removed from internal image cache
         if(!pInfo->isVideo) removeImageFromCache(dirName, pInfo->filename);

         // remove entry for file from the TOC
         pFolder->pFirstMedia = pInfo->pNext;
         --(pFolder->nMediaFiles);
         ::free(pInfo);
      }

      // now remove the folder. Again, this could fail if there are any non-movie files still in the directory.
      ::sprintf(path, "%s/%s", MEDIASTOREDIR, dirName);
      if(0 != ::rmdir(path))
      {
         fprintf(stderr, "(CRMVMediaMgr::replyDeleteMediaFile) WARNING: Unable to remove directory corres to\n");
         fprintf(stderr, "   now-empty media folder '%s'. Directory may contain other non-media files\n", path);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
      
      // finally, remove folder from TOC
      if(pPrevFolder == NULL) m_pFirstMediaFolder = pFolder->pNext;
      else pPrevFolder->pNext = pFolder->pNext;
      --m_nMediaFolders;
      ::free(pFolder);
   }
   
   pIOLink->sendSignal(RMV_SIG_CMDACK);
}

/**
 Download a media file over the Maestro-RMVideo communication link in response to the RMV_CMD_PUTFILE command. It is
 assumed that RMV_CMD_PUTFILE was just sent, and that the names of the destination file and its parent folder are
 available via CRMVIo::getMediaFile() and getMediaFolder(), respectively.
 
 If the specified media folder does not exist in the media store, the method will create the folder and fail if it
 cannot do so. If the media store already has RMV_MVF_LIMIT folders, it cannot add another and the method fails. If
 the folder already exists, the method opens a new file in the location specified. If a file already exists there, if
 the folder already contains the maximum number of media files allowed (RMV_MVF_LIMIT), or if the system could not
 open the new file, the method fails. Upon failure, a RMV_SIG_CMDERR signal is returned to Maestro, terminating the 
 download before it begins. Otherwise, the method sends a RMV_SIG_CMDACK to Maestro to start the download and passes 
 the open file descriptor to the helper method CRMVIo::downloadFile(), which mediates the remainder of the file 
 transfer: a sequence of RMV_CMD_PUTFILECHUNK packets followed by one RMV_CMD_PUTFILEDONE. Upon return, this file 
 transfer either succeeded or failed. If it was successful, the downloaded file is examined to verify that it is a 
 media file that can be handled by RMVideo and to retrieve some video or image information for the table of contents. 
 If unable to validate the file, the file is removed and RMV_SIG_CMDERR is sent in reply to RMV_CMD_PUTFILEDONE. Else,
 the operation succeeds, a RMV_SIG_CMDACK is sent to Maestro, and an entry for the media file is added to the media
 store manager's internal table of contents.

 @param pIOLink The Maestro-RMVideo comm link over which media file download will occur.
 @param dirName Name of the media folder into which file should be downloaded.
 @param fName Name of the media file to be downloaded.
*/
void CRMVMediaMgr::downloadMediaFile(CRMVIo* pIOLink)
{
   const char* dirName = pIOLink->getMediaFolder();
   const char* fName = pIOLink->getMediaFile();

   char dirPath[256];
   char filePath[256];
   
   // find the media folder specified. If not found in our TOC nor on the file system, then create the corresponding
   // directory on the file system.
   if(!m_bLoaded) 
   {
      ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Media store not initialized!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }

   // allocate a TOC entry for the file to be downloaded
   MediaInfo* pNewInfo = (MediaInfo*) ::malloc(sizeof(MediaInfo));
   if(pNewInfo == NULL)
   {
      ::perror("(CRMVMediaMgr::downloadMediaFile) Memory allocation failed!\n");
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   MediaFolder* pFolder = m_pFirstMediaFolder;
   while(pFolder != NULL)
   {
      if(::strcmp(dirName, pFolder->name) == 0) break;
      pFolder = pFolder->pNext;
   }
   
   // if folder does not exist, try to create it. Otherwise, make sure there's not a media file in that folder with
   // the same name; if so, fail.
   bool folderCreated = false;
   if(pFolder == NULL)
   {
      if(m_nMediaFolders == RMV_MVF_LIMIT)
      {
         fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Cannot add a new media folder -- capacity reached!\n");
         ::free(pNewInfo);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
      void removeImageFromCache(const char* folder, const char* file);
      pFolder = (MediaFolder*) ::malloc(sizeof(MediaFolder));
      if(pFolder == NULL)
      {
         ::perror("(CRMVMediaMgr::downloadMediaFile) Memory allocation failed!\n");
         ::free(pNewInfo);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
      ::sprintf(dirPath, "%s/%s", MEDIASTOREDIR, dirName);
      if(0 != ::mkdir(dirPath, 0777))
      {
         ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Unable to create directory at '%s'\n", dirPath);
         ::free(pFolder);
         ::free(pNewInfo);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
      
      ::strcpy(pFolder->name, dirName);
      pFolder->nMediaFiles = 0;
      pFolder->pFirstMedia = NULL;
      pFolder->pNext = NULL;
      folderCreated = true;
   }
   else
   {
      // make sure folder is not full
      if(pFolder->nMediaFiles == RMV_MVF_LIMIT)
      {
         ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Destination folder '%s' is full!\n", dirName);
         ::free(pNewInfo);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;void removeImageFromCache(const char* folder, const char* file);
      }
      
      // make sure there's not a media file in that folder with the same name. If so, fail.
      MediaInfo* pInfo = pFolder->pFirstMedia;
      while(pInfo != NULL)
      {
         if(::strcmp(fName, pInfo->filename) == 0) break;
         pInfo = pInfo->pNext;
      }
      if(pInfo != NULL)
      {
         ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Destination file '%s/%s' already exists!\n",
                   dirName, fName);
         ::free(pNewInfo);
         pIOLink->sendSignal(RMV_SIG_CMDERR);
         return;
      }
   }

   // open a new file in the location specified in the media store. Make sure no such file already exists -- in case
   // the TOC is out of sync!
   ::sprintf(filePath, "%s/%s/%s", MEDIASTOREDIR, dirName, fName);
   FILE* fd = ::fopen(filePath, "wx");
   if(fd == NULL)
   {
      ::fprintf(stderr, 
	   "(CRMVediaMgr::downloadMediaFile) Failed to open new file at '%s'; file already there?\n", filePath);
      if(folderCreated)
      {
         ::free(pFolder);
         ::rmdir(dirPath);
      }
      ::free(pNewInfo);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
   
   // send ack in response to RMV_CMD_PUTFILE, initiating download
   pIOLink->sendSignal(RMV_SIG_CMDACK);
   
   // download the file. Successful or not, the file descriptor is closed. Upon success, Maestro is still waiting for
   // a reply to the RMV_CMD_PUTFILEDONE command that completed the download. On failure, the IO link has already sent
   // a reply to Maestro, and all we have to do is remove the partial file if it's there.
   if(!pIOLink->downloadFile(fd))
   {
      ::remove(filePath);
      if(folderCreated)
      {
         ::free(pFolder);
         ::rmdir(dirPath);
      }
      ::free(pNewInfo);
      return;
   }
   
   // file was successfully downloaded. Now ensure that RMVideo recognizes it as an image or video file that it can
   // handle, and extract some info about the video or image for inclusion in the TOC. Preload and cache an image.
   ::strcpy(pNewInfo->filename, fName);
   pNewInfo->pNext = NULL;
   pNewInfo->isVideo = false;
   pNewInfo->width = pNewInfo->height = pNewInfo->rate = pNewInfo->dur = 0;
   bool bOk = true;
   if(CRMVMediaMgr::getImageInfo(filePath, pNewInfo->width, pNewInfo->height))
   {
      bOk = (pNewInfo->width > 0) && (pNewInfo->width <= MAX_IMAGEDIM) && 
            (pNewInfo->height > 0) && (pNewInfo->height <= MAX_IMAGEDIM);
      if(bOk) bOk = (NULL != addImageToCache(dirName, fName));
   }
   else if(CVidBuffer::getVideoInfo(filePath, pNewInfo->width, pNewInfo->height, pNewInfo->rate, pNewInfo->dur, true))
   {
      pNewInfo->isVideo = true;
      bOk = (pNewInfo->width > 0) && (pNewInfo->height > 0);
   }
   else bOk = false;
   
   if(!bOk)
   {
      ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Cannot read downloaded media file, or file format is \n");
      ::fprintf(stderr, "   not supported, or image W or H exceeds %ld. Deleting %s...\n", MAX_IMAGEDIM, filePath);
      ::remove(filePath);
      if(folderCreated)
      {
         ::free(pFolder);
         ::rmdir(dirPath);
      }
      ::free(pNewInfo);
      pIOLink->sendSignal(RMV_SIG_CMDERR);
      return;
   }
      
   pIOLink->sendSignal(RMV_SIG_CMDACK);

   ::fprintf(stderr, "(CRMVMediaMgr::downloadMediaFile) Media file successfully downloaded to %s. Stats:\n", filePath);
   if(pNewInfo->isVideo)
      ::fprintf(stderr, "  %d x %d frame size in pixels; %.3f Hz; %.3f seconds.\n", pNewInfo->width, pNewInfo->height,
             ((double) pNewInfo->rate) / 1000.0, ((double) pNewInfo->dur) / 1000.0);
   else
      ::fprintf(stderr, "  %d x %d image size in pixels.\n", pNewInfo->width, pNewInfo->height);
   
   // success. Add entry to TOC for the new file and, if necessary, a new media folder. Then ack success w/Maestro.
   if(folderCreated)
   {
      if(m_pFirstMediaFolder == NULL) m_pFirstMediaFolder = pFolder;
      else
      {
         MediaFolder* pLastFolder = m_pFirstMediaFolder;
         while(pLastFolder->pNext != NULL) pLastFolder = pLastFolder->pNext;
         pLastFolder->pNext = pFolder;
      }
      ++m_nMediaFolders;
   }

   if(pFolder->pFirstMedia == NULL) pFolder->pFirstMedia = pNewInfo;
   else
   {
      MediaInfo* pLast = pFolder->pFirstMedia;
      while(pLast->pNext != NULL) pLast = pLast->pNext;
      pLast->pNext = pNewInfo;
   }
   ++(pFolder->nMediaFiles);
}

/**
 Retrieve an image from the media store. 

 NOTE: Images are cached internally by CRMVMediaMgr to optimize access to very large images (e.g., 2560x1440). It is
 imperative that callers never free() the image buffer pointer returned by this method, nor keep a reference to it --
 since the image data could be released at any time if the cache grows too large.

 @param folder Name of media folder containing image source file.
 @param file Name of image source file.
 @param width [out] On successful return, contains image width in pixels; else, 0.
 @param height [out] On successful return, contains image height in pixels; else, 0.
 @return A pointer to the image data buffer. Returns NULL on failure, in which case a very brief error description is 
 printed to stderr. 
*/
unsigned char* CRMVMediaMgr::getImage(const char* folder, const char* file, int& width, int& height)
{
   // try the image cache first. If it's not cached, load the image and cache it
   CachedImage* cachedImg = retrieveImageFromCache(folder, file);
   if(cachedImg == NULL) cachedImg = addImageToCache(folder, file);

   if(cachedImg == NULL)
   {
      width = height = 0;
      return(NULL);
   }
   
   width = cachedImg->wPix;
   height = cachedImg->hPix;
   return(cachedImg->pImgBuf);
}

/**
 Reallocate the integer reply buffer if it is not large enough to accommodate the requested size.
 @param sz The reply buffer size needed.
 @return True if reply buffer was already large enough or was reallocated to accommodate the specified size. If it
 was not reallocated successfully, the reply buffer will be NULL and false is returned.
*/
bool CRMVMediaMgr::ensureSufficientReplyBufSize(int sz)
{
   if(sz < m_nReplyBufSz) return(true);

   if(m_pReplyBuf != NULL) ::free(m_pReplyBuf);
   m_nReplyBufSz = 0;
      
   m_pReplyBuf = (int*) ::calloc(sz + 100, sizeof(int));
   if(m_pReplyBuf == NULL) return(false);
      
   m_nReplyBufSz = sz + 100;
   return(true);
}

/**
 Helper method for load(). Appends a media folder to the TOC structure with the specified name. Does NOT check to see
 if folder name matches that of an existing folder.
 @param folderName Name of folder to add.
 @return True if successful, false if memory allocation failed (msg printed to stderr). If store already contains the
 maximum number of folders allowed, method returns true but takes no action. Caller should check that there's room
 for another folder before calling this method.
*/
bool CRMVMediaMgr::appendMediaFolder(const char* folderName)
{
   // if we reached limit, do not append the new folder, but return true b/c this is not a fatal error.
   if(m_nMediaFolders == RMV_MVF_LIMIT)
   {
      fprintf(stderr, "CRMVMediaMgr::appendMediaFolder) Media store full. Cannot add another media folder!\n");
      return(true);
   }
   
   MediaFolder* pFolder = (MediaFolder*) ::malloc(sizeof(MediaFolder));
   if(pFolder == NULL)
   {
      ::perror("(CRMVMediaMgr::appendMediaFolder) Memory allocation failed!\n");
      return(false);
   }
   
   ::strcpy(pFolder->name, folderName);
   pFolder->nMediaFiles = 0;
   pFolder->pFirstMedia = NULL;
   pFolder->pNext = NULL;
   
   if(m_pFirstMediaFolder == NULL) m_pFirstMediaFolder = pFolder;
   else
   {
      MediaFolder* pPrev = m_pFirstMediaFolder;
      while(pPrev->pNext != NULL) pPrev = pPrev->pNext;
      pPrev->pNext = pFolder;
   }
   ++m_nMediaFolders;
   
   return(true);
}

/**
 Helper method for load(). Scans all files in the specified media folder and appends an entry in the TOC for each
 such file that can is recognized either as a supported video file (via CVidBuffer::getVideoInfo()) or as a supported
 image file (via CRMVMediaMgr::getImageInfo()). Assumes that the directory corresponding to the folder already exists 
 in the media store. Scan stops once RMV_MVF_LIMIT media files have been found in the directory.

 To optimize access to very large (fullscreen) images, the method preloads images into an internal cache (until that
 cache reaches capacity). Since image loading is relatively slow (1-2 seconds for a 2560x1440 image), this method 
 could take some to complete if the media folder contains many large images.
 
 @param pFolder The media folder to scan.
 @return True if successful, false if memory allocation failed or unable to scan directory (msg printed to stderr).
 Does not fail if an unrecognized file is encountered; such files are simply "skipped over".
 */
bool CRMVMediaMgr::scanMediaFolder(MediaFolder* pFolder)
{
   char path[100];
   ::sprintf(path, "%s/%s", MEDIASTOREDIR, pFolder->name);
   
   DIR* pDir = ::opendir(path);
   if(pDir == NULL)
   {
      fprintf(stderr, "(CRMVMediaMgr::scanMediaFolder) Unable to scan directory: %s\n", path);
      return(false);
   }
   
   // find the last media file currently in the TOC for the folder (if any). If the folder is already full, do not scan.
   if(pFolder->nMediaFiles == RMV_MVF_LIMIT)
   {
      ::closedir(pDir);
      return(true);
   }
   MediaInfo* pPrev = pFolder->pFirstMedia;
   if(pPrev != NULL) while(pPrev->pNext != NULL) pPrev = pPrev->pNext;
   
   struct stat statInfo;
   struct dirent* pDirEntry = ::readdir(pDir);
   while(pDirEntry != NULL)
   {
      // skip over "." and ".." directory entries
      bool append = (0 != ::strcmp(pDirEntry->d_name, ".")) && (0 != ::strcmp(pDirEntry->d_name, ".."));
      
      // ensure file name meets RMVideo requirements and is a regular file
      if(append) 
      {
         int len = ::strlen(pDirEntry->d_name);
         append = (len > 0) && (len <= RMV_MVF_LEN) && (len == ::strspn(pDirEntry->d_name, RMV_MVF_CHARS));
      }
      if(append)
      {
         ::sprintf(path, "%s/%s/%s", MEDIASTOREDIR, pFolder->name, pDirEntry->d_name);
         append = (0 == ::stat(path, &statInfo));
         if(append) append = S_ISREG(statInfo.st_mode);
      }
      
      // check that file is a supported image or video file
      int type = -1;   // 0 = image, 1 = video
      int width = 0, height = 0, rate = 0, dur = 0;
      if(append)
      {
         if(CRMVMediaMgr::getImageInfo(path, width, height) && width > 0 && height > 0)
         {
            type = 0;
            if(width > MAX_IMAGEDIM || height > MAX_IMAGEDIM)
            {
               append = false;
               fprintf(stderr, "(CRMVMediaMgr::scanMediaFolder) Ignoring file '%s': Image is too large.\n", path);
            }
         }
         else if(CVidBuffer::getVideoInfo(path, width, height, rate, dur, false)) type = 1;
         else
         {
            append = false;
            fprintf(stderr, "(CRMVMediaMgr::scanMediaFolder) Ignoring file '%s': Not a supported media file.\n", path);
         }
      }
      
      // if it's a supported image file, preload the image unless the image cache has reached capacity. If we are unable
      // to load image, that's an indication the image file is not valid.
      if(append && type==0 && ((m_nCacheSize + width*height*4) < MAX_IMGCACHESZ))
      {
         CachedImage* pCachedImg = addImageToCache(pFolder->name, pDirEntry->d_name);
         if(pCachedImg == NULL)
         {
            fprintf(stderr, "(CRMVMediaMgr::scanMediaFolder) Ignoring image '%s': Failed to load image data.\n", path);
            append = false;
         }
      }

      // if we've found a supported media file, append a TOC entry for it
      if(append)
      {
         MediaInfo* pInfo = (MediaInfo*) ::malloc(sizeof(MediaInfo));
         if(pInfo == NULL)
         {
            ::perror("(CRMVMediaMgr::scanMediaFolder) Memory allocation failed!\n");
            ::closedir(pDir);
            return(false);
         }
         
         ::strcpy(pInfo->filename, pDirEntry->d_name);
         pInfo->pNext = NULL;
         pInfo->isVideo = (type == 1) ? true : false;
         pInfo->width = width;
         pInfo->height = height;
         pInfo->rate = rate;
         pInfo->dur = dur;
         
         if(pPrev == NULL) pFolder->pFirstMedia = pInfo;
         else pPrev->pNext = pInfo;
         pPrev = pInfo;
         
         ++(pFolder->nMediaFiles);
      }
      
      // proceed to next file, unless we've reached capacity for this folder
      pDirEntry = ::readdir(pDir);
      if(pDirEntry != NULL && pFolder->nMediaFiles == RMV_MVF_LIMIT)
      {
         fprintf(stderr, "(CRMVMediaMgr::scanMediaFolder) Media folder '%s' is full.\n", pFolder->name);
         break;
      }
   }
   ::closedir(pDir);

   return(true);
}

/**
 Check whether or not the specified file is a supported image file and, if so, retrieve the image width and height.
 @param path The file path relative to RMVideo's installation directory.
 @param width [out] On success, will contain the image width in pixels.
 @param height [out] On success, will contain the image height in pixels.
 @return True if specified file is found and is a supported image file; else false.
*/
bool CRMVMediaMgr::getImageInfo(const char* path, int& width, int& height)
{
   int nComp = 0;
   return(0 != ::stbi_info(path, &width, &height, &nComp));
}

/**
 Load the image data from the specified image file. Regardless the internal format (monochrome, RGB, RGBA), the
 method returns an image buffer in RGBA format -- so transparency is supported. If the original is opaque, each 
 pixel's alpha channel will be 1.0. The image buffer should be in a form that can be supplied directly to 
 glTexImage2D() with the GL_RGBA pixel data format and GL_UNSIGNED_BYTE pixel data type. 

 The image buffer returned is allocated on the heap, so be sure to release it via CRMVMediaMgr::freeImageData() when
 it is no longer needed.

 @param path The image file path relative to RMVideo's installation directory.
 @param width [out] On success, will contain the image width in pixels.
 @param height [out] On success, will contain the image height in pixels.
 @return A pointer to the image data buffer. Returns NULL on failure. Note that a very brief error description is
 printed to stderr. This might help user determine if a file cannot be read via STBI_IMAGE.H.
*/
unsigned char* CRMVMediaMgr::loadImageData(const char* path, int& width, int& height)
{
   ::stbi_set_flip_vertically_on_load(true);  // because OpenGL texture Y-coord is flipped WRT image coordinates
   int nComp = 0;
   unsigned char *buf = (unsigned char *) ::stbi_load(path, &width, &height, &nComp, STBI_rgb_alpha);
   if(buf==NULL)
      ::fprintf(stderr, "Image load failed for file at '%s': %s\n", path, ::stbi_failure_reason());
   return(buf);
}

/**
 Release heap memory allocated for image data previously loaded via loadImageData().
 @param pImgData Pointer to the image data buffer to be freed. If NULL, no action is taken.
*/
void CRMVMediaMgr::freeImageData(unsigned char* pImgData)
{
   if(pImgData != NULL) ::stbi_image_free((void*) pImgData);
}


/** Empty the internal image cache. Be sure to call this before RMVideo exits. */
void CRMVMediaMgr::releaseImageCache()
{
   while(m_pImageCache != NULL)
   {
      CachedImage* pDead = m_pImageCache;
      m_pImageCache = pDead->pNext;
      pDead->pNext = NULL;

      CRMVMediaMgr::freeImageData(pDead->pImgBuf);
      ::free(pDead);
      pDead = NULL;
   }
   m_nCachedImages = 0;
   m_nCacheSize = 0L;
}

/**
 Retrieve the specified image from the internal, in-memory image cache.

 @param folder [in] The name of the media store folder containing the image source file.
 @param file [in] The name of the image source file.
 @return The cached image object, including access to the GL_RGBA image data buffer. NULL if image not cached.
*/
CRMVMediaMgr::CachedImage* CRMVMediaMgr::retrieveImageFromCache(const char* folder, const char* file)
{
   CachedImage* pFound = m_pImageCache;
   while(pFound != NULL)
   {
      if(::strcmp(pFound->folderName, folder)==0 && ::strcmp(pFound->fileName, file)==0) break;
      pFound = pFound->pNext;
   }
   return(pFound);
}

/**
 Load an image from the specified source file in the media store and add it to an internal, in-memory cache.

 The image cache is implemented as a singly-linked list of image "slots", with new images appended to the end of the 
 list. The cache will grow until it reaches a total capacity of MAX_IMGCACHESZ, at which point the oldest image(s) are
 removed from the head of the cache until there's room for the specified image.

 See also: CRMVMediaMgr::loadImageData().

 @param folder [in] The name of the media store folder containing the image source file.
 @param file [in] The name of the image source file.
 @return NULL if operation fails; else the allocated image object, including access to the GL_RGBA image data buffer.
*/
CRMVMediaMgr::CachedImage* CRMVMediaMgr::addImageToCache(const char* folder, const char* file)
{
   // just in case it is already cached
   CachedImage* pCached = retrieveImageFromCache(folder, file);
   if(pCached != NULL) return(pCached);

   // load the image data -- abort if we can't
   char path[100];
   ::sprintf(path, "%s/%s/%s", MEDIASTOREDIR, folder, file);

   int w = 0, h = 0;
   unsigned char *buf = CRMVMediaMgr::loadImageData(path, w, h);
   if(buf == NULL) return(NULL);

   // if adding image would cause cache to exceed capacity, remove the oldest images in the cache (at HEAD of list)
   // until there's enough room.
   while(m_nCacheSize + w*h*4 > MAX_IMGCACHESZ)
   {
      CachedImage* pDead = m_pImageCache; 
      m_pImageCache = pDead->pNext;
      --m_nCachedImages;
      m_nCacheSize -= ((unsigned long) pDead->wPix) * ((unsigned long) pDead->hPix) * 4L;

      CRMVMediaMgr::freeImageData(pDead->pImgBuf);
      ::free(pDead);
      pDead = NULL;
   }

   // append new image at the end of the cache
   CachedImage* pImg = (CachedImage*) ::malloc(sizeof(CachedImage));
   if(pImg == NULL)
   {
      CRMVMediaMgr::freeImageData(buf);
      ::perror("(CRMVMediaMgr::addImageToCache) Memory allocation failed!\n");
      return(NULL);
   }
   ::strcpy(pImg->folderName, folder); 
   ::strcpy(pImg->fileName, file);
   pImg->wPix = w;
   pImg->hPix = h;
   pImg->pImgBuf = buf;
   pImg->pNext = NULL;

   if(m_pImageCache == NULL) m_pImageCache = pImg;
   else
   {
      CachedImage* pBefore = m_pImageCache;
      while(pBefore->pNext != NULL) pBefore = pBefore->pNext;
      pBefore->pNext = pImg;
   }
   ++m_nCachedImages;
   m_nCacheSize += ((unsigned long) w) * ((unsigned long) h) * 4L;

   return(pImg);
}

/**
 Remove the specified image from the in-memory image cache, if it is there.

 @param folder [in] The name of the media store folder containing the image source file.
 @param file [in] The name of the image source file.
*/
void CRMVMediaMgr::removeImageFromCache(const char* folder, const char* file)
{
   // find the cached image, if it's there
   CachedImage* pBefore = NULL;
   CachedImage* pRmv = m_pImageCache;
   while(pRmv != NULL)
   {
      if(::strcmp(pRmv->folderName, folder)==0 && ::strcmp(pRmv->fileName, file)==0) break;
      pBefore = pRmv;
      pRmv = pRmv->pNext;
   }
   if(pRmv == NULL) return;

   // excise the slot for the image to be removed
   if(pBefore == NULL) m_pImageCache = pRmv->pNext;
   else pBefore->pNext = pRmv->pNext;
   pRmv->pNext = NULL;

   CRMVMediaMgr::freeImageData(pRmv->pImgBuf);
   ::free(pRmv);
   pRmv = NULL;
}

