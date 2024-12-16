//=====================================================================================================================
//
// rmvmediamgr.h : Declaration of class CRMVMediaMgr
//
//=====================================================================================================================

#if !defined(MEDIAMGR_H__INCLUDED_)
#define MEDIAMGR_H__INCLUDED_

#include "rmvio.h"                     // CRMVIo -- Defines the communication link with Maestro.
#include "rmvideo_common.h"            // common defns shared by Maestro and RMVideo


class CRMVMediaMgr
{
public:
   // subdirectory of RMVideo home directory in which all media files are stored
   static const char* MEDIASTOREDIR;
   
   CRMVMediaMgr();
   ~CRMVMediaMgr();

   // scan media storage directory and prepare table of contents
   bool load();
   
   // handle Maestro commands related to media file storage
   void replyGetMediaDirs(CRMVIo* pIOLink);
   void replyGetMediaFiles(CRMVIo* pIOLink);
   void replyGetMediaInfo(CRMVIo* pIOLink);
   void replyDeleteMediaFile(CRMVIo* pIOLink);
   void downloadMediaFile(CRMVIo* pIOLink);
   
   // retrieve a GL_RGBA-formatted image from the media store (checks image cache first for fast retrieval)
   unsigned char* getImage(const char* folder, const char* file, int& width, int& height);

private:
   // name of subdirectory in which video files were stored in RMVideo version 6 or earlier
   static const char* OLDSTOREDIR;
   
   bool ensureSufficientReplyBufSize(int sz);         // to reallocate reply buffer as needed
   
   // information on a video or image file in RMVideo's media store
   struct MediaInfo
   {
      char filename[RMV_MVF_LEN+1];
      bool isVideo;                                   // TRUE for a video file, FALSE for an image file
      int width;                                      // width of image or video frame in pixels; 0 if unknown
      int height;                                     // height of image or video frame in pixels; 0 if unknown
      int rate;                                       // video frame rate in milliHz; 0 if unknown
      int dur;                                        // approximate video duration in ms; 0 if unknown
      MediaInfo* pNext;
   };
   
   // a folder in RMVideo's media store
   struct MediaFolder
   {
      char name[RMV_MVF_LEN+1]; 
      int nMediaFiles;
      MediaInfo* pFirstMedia;
      MediaFolder* pNext;
   };
   
   // helper methods for load()
   bool appendMediaFolder(const char* folderName);
   bool scanMediaFolder(MediaFolder* pFolder);

   bool m_bLoaded;                                    // flag set once we've scanned media store contents
   int m_nMediaFolders;                               // number of folders currently in media store
   MediaFolder* m_pFirstMediaFolder;                  // ptr to first folder in the media store
   int* m_pReplyBuf;                                  // buffer used to send data back to Maestro over comm link
   int m_nReplyBufSz;                                 // length of reply buffer (# of 32-bit ints)

   // static helper methods for dealing with images via STB_IMAGE header library
   static bool getImageInfo(const char* path, int& width, int& height);
   static unsigned char* loadImageData(const char* path, int& width, int& height);
   static void freeImageData(unsigned char* pImgData);

   // one slot in the singly-linked list of images cached in memory for fast retrieval
   struct CachedImage
   {
      char folderName[RMV_MVF_LEN+1];            // media store folder name for image source file
      char fileName[RMV_MVF_LEN+1];              // image source filename
      int wPix;                                  // image width in pixels
      int hPix;                                  // image height in pixels
      unsigned char* pImgBuf;                    // the image buffer, as prepared by loadImageData()
      CachedImage* pNext;                        // points to next image slot in cache
   };

   static const unsigned long MAX_IMGCACHESZ;    // max amount of memory allocated to cached images (in bytes)
   static const unsigned long MAX_IMAGEDIM;      // max allowed width or height of an image

   // managing internal image cache to speed up retrieval of very large (e.g., 2560x1440) images
   void releaseImageCache();
   CachedImage* retrieveImageFromCache(const char* folder, const char* file);
   CachedImage* addImageToCache(const char* folder, const char* file);
   void removeImageFromCache(const char* folder, const char* file);

   unsigned int m_nCachedImages;                 // number of images stored in cache
   unsigned long m_nCacheSize;                   // total size of image cache in bytes
   CachedImage* m_pImageCache;                   // pointer to first image in cache
};


#endif   // !defined(MEDIAMGR_H__INCLUDED_)
