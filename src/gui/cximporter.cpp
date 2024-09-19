//===================================================================================================================== 
//
// cximporter.cpp : Implementation of CCxImporter, which encapsulates the process of importing ASCII-text cntrlxUNIX 
//                  object definition into the Maestro experiment document framework.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Maestro's predecessor was known as cntrlxUNIX/PC, where cntrlxUNIX referred to the UNIX workstation-based application 
// that displayed the GUI and communicated with a separate PC-based app, cntrlxPC, that controlled the rig hardware.
// In that system, the user could save experimental protocols in ASCII-text definition files. There were separate 
// files for targets, trials, stimulus run sets, perturbations, and channel configurations, and each file type had a 
// different format. 
//
// To facilitate the transition from cntrlxUNIX/PC to Maestro, we required a means of "importing" these old-style 
// definition files into the Maestro experiment document. CCxImporter encapsulates this import process.
// 
// ==> Usage.
// Merely construct a CCxImporter object and call DoImport(), passing it the full pathname of an "import" directory.
// The import process, as structured in CCxImporter, expects this directory to be organized as follows:
//
//    settings.ini   ==> One or more application settings (video display parameters, fix/reward settings, etc) are 
//       defined in this file, if it exists. This file follows the format of the cntrlxUNIX "video display settings" 
//       text definition file, but it includes additional keywords for defining other Maestro application settings. 
//       For details, see CCxSettings::Import(). Note that there is only one application settings object associated 
//       with the current Maestro experiment document, and this object is NOT part of the object tree.
//    perts.ini      ==> All perturbation objects to be imported are defined in this file, if it exists.
//    channels.ini   ==> All channel configs to be imported are defined in this file, if it exists.
//    \targets       ==> CCxImporter searches files in this folder and its immediate subfolders. If a subfolder 
//       contains at least one valid target import file, a Maestro target set object is created and named after the  
//       subfolder. Each target file within the subfolder is imported as a target object within the set. Any target 
//       files in the \targets folder itself are added to the root of the Maestro "Targets" subtree. 
//    \trials        ==> CCxImporter searches files in this folder and its immediate subfolders. If a subfolder 
//       contains at least one valid trial file, a Maestro trial set object is created and named after the subfolder. 
//       Each trial file within the subfolder is imported as a trial object within the set. Any trial files in the 
//       \trials folder itself are added to a trial set object called "importSet". 
//    \runs          ==> CCxImporter searches files in this folder and its immediate subfolders. If a subfolder 
//       contains at least one valid stimulus run file, a Maestro run set object is created and named after the 
//       subfolder. Each stimulus run (there can be one or more in each file) defined in the stimulus run file(s) 
//       within the subfolder is imported as a stimulus run object within the set. Any stimulus runs defined in run 
//       files in the \runs folder itself are added to a run set object called "importSet". 
//
// Since the import process may take a while, CCxImporter logs progress messages (via CCntrlxApp::LogMessage()) to keep 
// the user informed and displays the wait cursor. If it is unable to import a particular object, it will log an 
// appropriate error message.
//
// ==> Implementation notes.
//
// 1) During the import process, we build an array of keys for all objects created (member var m_wArKeys), and an 
// "import map" (member var m_importMap).  Saving the imported keys temporarily allows the user to cancel the import 
// after it has finished. The import map stores (cntrlxUNIX name, Maestro key)-pairs for each "independent object" 
// (channel cfg, perturbation, or target) that is successfully imported. Each independent object in a cntrlxUNIX trial 
// definition is identified by its name; in a Maestro trial, such objects are identified instead by their unique keys. 
// While importing a trial, we use the import map to find the key of the imported Maestro object corresponding to each 
// object name in the cntrlxUNIX trial definition. Of course, for this scheme to work, CCxImporter must import all 
// independent objects before importing any trials.
//
// 2) Each Maestro data class (CCxChannel, CCxPert, etc.) provides an Import() method which imports a cntrlxUNIX defn 
// stored as an array of text lines.  
//
// 3) The CCxTrial::Import() method expects a CMapStrToWord collection representing the import map. For each trial 
// object created, CCxImporter must update the trial's "dependencies" in order to properly lock any other objects upon 
// which that trial's definition depends.  See ImportTrialFile().
//
//
// REVISION HISTORY:
// 08jan2003-- Created (moved code from CCntrlxApp and adapted).
// 30sep2003-- Modified to import application settings defined in file "settings.ini" in the import directory.
// 23sep2011-- Removed support for importing trials or stimulus runs that use the now-obsolete targets CX_FIBER1/2 and
// CX_REDLED1/2. Effective Maestro 3.0.
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017. Also, eliminated use of 
// old CMapStringToWord class and replaced with CMap<CString, LPCTSTR, WORD, WORD>.
//===================================================================================================================== 


#include "stdafx.h"                          // standard MFC stuff

#include "cntrlx.h"                          // CCntrlxApp -- the Maestro app
#include "cxdoc.h"                           // CCxDoc -- the Maestro experiment document
#include "cxviewhint.h"                      // CCxViewHint -- for informing views of doc changes after import
#include "cxpert.h"                          // CCxPert -- the Maestro perturbation data class
#include "cxchannel.h"                       // CCxChannel -- the Maestro channel configuration data class
#include "cxtarget.h"                        // CCxTarget -- the Maestro target data class
#include "cxtrial.h"                         // CCxTrial -- the Maestro trial data class
#include "cxcontrun.h"                       // CCxContRun -- the Maestro stimulus run data class

#include "cximporter.h"

#ifdef _DEBUG
#define new DEBUG_NEW                        // version of new operator for detecting memory leaks in debug release. 
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 

//=== DoImport ======================================================================================================== 
//
//    Import cntrlxUNIX text definition files found in the specified import directory.  The import directory must be 
//    laid out as described in the header of this implementation file.  Progress and/or error messages are logged with 
//    the application via CCntrlxApp::LogMessage().
//
//    ARGS:       szDir -- [in] full pathname of the import directory.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::DoImport( LPCTSTR szDir )
{
   int i;

   CCxDoc* pDoc = ((CCntrlxApp*)AfxGetApp())->GetDoc();     // the current Maestro experiment document
   BOOL bWasModified = pDoc->IsModified();                  // was document modified prior to starting import?

   CWaitCursor waitCursor;                                  // this operation could take a while!

   m_strDir = szDir;                                        // initializations
   m_wArKeys.RemoveAll();
   m_importMap.RemoveAll();

   // create entries in the import map for the animal chair and the "Default" channel configuration. These exist as 
   // predefined object in Maestro and are not imported.
   // NOTE: As of Maestro 3.0, predefined targets CX_FIBER* and CX_REDLED* are no longer supported. As of Maestro 1.5,
   // the CX_OKNDRUM target is no longer supported. If any defined trials or stimulus runs use any of these, they will
   // fail to import...
   m_importMap.SetAt(_T("turntable"), pDoc->GetChairTarget());
   m_importMap.SetAt(_T("Default"), pDoc->GetDefaultChannelConfig());

   // import application settings, if settings file exists
   BOOL bNewSettings = ImportSettings(); 

   // import all objects in text defn files, "independent" objects first, building import map as we go. Status messages
   // reflect progress.
   ImportChannels(); 
   ImportPerts(); 
   ImportTargets();  
   ImportTrials(); 
   ImportRuns(); 

   // if we imported anything, make sure the user wants to commit the imported objects.
   if(bNewSettings || (m_wArKeys.GetSize() > 0))
   {
      int iRes = ::AfxMessageBox(_T("Commit the import?"), MB_YESNO|MB_ICONQUESTION|MB_APPLMODAL);

      if(iRes == IDNO) 
      {  
         // if not, remove all imported objects, restoring doc to previous state. Since independent objects are deleted
         // first, we won't have any "dependency" problems.
         for(i=0; i < m_wArKeys.GetSize(); i++) pDoc->RemoveObj(m_wArKeys[i]);
         pDoc->SetModifiedFlag(bWasModified);
      }
      else 
      {
         // if so, tell other views that objects have been added
         CCxViewHint vuHint(CXVH_NEWOBJ, 0, CX_NULLOBJ_KEY); 
         pDoc->UpdateAllViews(NULL, LPARAM(0), (CObject*)&vuHint);

         // if app settings were imported, make them the current app settings inform all views of the change
         if(bNewSettings)  
         {
            CCxSettings* pStgs = pDoc->GetSettings();
            pStgs->Copy( m_importedSettings );
            pDoc->SetModifiedFlag( TRUE );
            vuHint.Set( CXVH_VIDEOSETTINGS, 0, CX_NULLOBJ_KEY );
            pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*)&vuHint );
            vuHint.Set( CXVH_FIXREWSETTINGS, 0, CX_NULLOBJ_KEY );
            pDoc->UpdateAllViews( NULL, LPARAM(0), (CObject*)&vuHint );
         }
      }
   }
   else
   {
      // nothing imported, but we will have changed document if we added a set obj and later removed it!
      pDoc->SetModifiedFlag(bWasModified);
   }
}



//===================================================================================================================== 
// IMPLEMENTATION
//===================================================================================================================== 

//=== ImportChannels ================================================================================================== 
//
//    If the import directory contains the file "settings.ini", import all application settings defined therein into a 
//    copy of the current document's application settings object (we cannot import directly into the document's 
//    settings object, since user could later decide NOT to commit the import!). 
//
//    ARGS:       NONE.
//
//    RETURNS:    TRUE if application settings were successfully imported; FALSE otherwise.
//
//    THROWS:     CMemoryException
//
BOOL CCxImporter::ImportSettings()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString str = m_strDir + _T("\\settings.ini");                 // try to open settings file in current import dir 
   CStdioFile file;
   if( !file.Open( str, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      pApp->LogMessage( _T("Could not find or open settings.ini") );
      return( FALSE );
   }

   CStringArray strArDef;                                         // set of text lines defining tgt obj to import
   CString strMsg;                                                // error message

   BOOL bOk = TRUE;                                               // TRUE if import successful
   try                                                            // copy entire file line by line into string array
   {
      while( file.ReadString( str ) ) strArDef.Add( str );
   }
   catch( CFileException *e )
   {
      bOk = FALSE;
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      str = _T("File I/O error on settings.ini: ");
      str += szErr;
      pApp->LogMessage( str );
   }

   // close the file -- we'll ignore exceptions for now 
   try { file.Close(); } catch (CFileException *e) { e->Delete(); } 

   if( bOk )                                                      // if we read file successfully, try to import it 
   {                                                              // into a copy of the app settings object
      m_importedSettings.Copy( *(pDoc->GetSettings()) ); 
      bOk = m_importedSettings.Import( strArDef, strMsg );
      if( bOk )
         pApp->LogMessage( _T("Imported application settings from settings.ini") );
      else
      {
         str = _T("Failed to import settings.ini: ");
         str += strMsg;
         pApp->LogMessage( str );
      }
   }

   return( bOk );
}


//=== ImportChannels ================================================================================================== 
//
//    If the import directory contains the file "channels.ini", import all channel configuration definitions (there 
//    could be more than one) in the file into new CCxChannel objects under the CNTRLX "Channels" subtree.  Each 
//    channel config that is successfully imported is registered in the array of imported keys and the import map.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportChannels()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString str = m_strDir + _T("\\channels.ini");                 // try to open channel cfg file in current import dir 
   CStdioFile file;
   if( !file.Open( str, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      pApp->LogMessage( _T("Could not find or open channels.ini") );
      return;
   }

   BOOL bOk = TRUE; 
   int nCfgs = 0;                                                 // #channel configs defined in file
   int nRead = -1;                                                // #channel config blocks read from file thus far
   int nImported = 0;                                             // #channel cfgs successfully imported from file 
   TCHAR szName[CX_MAXOBJNAMELEN];                                // cntrlxUNIX name of chan cfg being imported
   CStringArray strArDef;                                         // set of text lines defining chan cfg being imported 
   CString strMsg;                                                // error message
   WORD wKey;

   WORD wBase = pDoc->GetBaseObj( CX_CHANBASE );                  // all imported chan cfgs are inserted under this obj 
   ASSERT( wBase != CX_NULLOBJ_KEY );

   try                                                            // (CStdioFile::ReadString throws CFileException)
   {
      bOk = file.ReadString( str );                               // first line must be "CHANNELS"
      if( bOk ) bOk = BOOL(str == _T("CHANNELS")); 

      if( bOk ) bOk = file.ReadString( str );                     // 2nd line is usu. "INCLUDEDEF" -- ie, "Default" 
      if( bOk && str == _T("INCLUDEDEF") )                        // channel cfg is included in file; so we remove 
      {                                                           // CNTRLX's own default chan cfg from the import map 
         m_importMap.RemoveKey( _T("Default") );                  // and move to the next line 
         bOk = file.ReadString( str );
      }

      // 2nd or 3rd line must be "#CONDITIONS <N>", where <N> > 0 is the # of chan cfgs defined in the file
      if(bOk) bOk = BOOL(1 == ::sscanf_s(str, "#CONDITIONS %d", &nCfgs) && (nCfgs > 0));

      while( bOk && nRead < nCfgs )                               // BEGIN: import chan cfgs defined in file...
      {
         if( nRead < 0 )                                          //    special case: read "NAME ..." line for 1st blk
         {
            nRead = 0;
            bOk = file.ReadString( str );
            if( !bOk ) { strMsg = _T("Unexpected EOF"); break; }
         }

         // first line of block must be "NAME <nameStr>", with a non-empty <nameStr> that does not exceed a max length
         if( (str.GetLength() > CX_MAXOBJNAMELEN + 4) ||           
             (1 != ::sscanf_s( str, "NAME %s", szName, CX_MAXOBJNAMELEN)) ||        
             (0 >= ::strlen( szName )) ) 
         {
            strMsg = _T("Bad NAME specification");
            bOk = FALSE;
         }

         strArDef.RemoveAll();                                    //    read in rest of block defining the channel cfg
         while( bOk ) 
         {
            if( file.ReadString( str ) ) 
            {
               if( str.Find( _T("NAME") ) > -1 ) break;           //       we've reached start of next block!
               else strArDef.Add( str );
            }
            else                                                  //       we hit EOF while processing block...
            {
               bOk = BOOL(nRead >= nCfgs-1);                      //       ...an error only if we're NOT on last block
               if( !bOk ) strMsg = _T("Unexpected EOF");
               break;
            }
         }

         if( bOk )                                                //    attempt import: 
         {
            if( m_importMap.Lookup( szName, wKey ) )              //    if cxUNIX name already in import map, then we 
            {                                                     //    cannot import the channel cfg obj; however, we 
               strMsg = _T("Skipped over chan cfg ");             //    do not fail in this case -- we merely skip over 
               strMsg += szName;                                  //    this particular chan cfg
               strMsg += _T("; name already imported!");
               pApp->LogMessage( strMsg );
               ++nRead;
            }
            else 
            {
               wKey = pDoc->InsertObj(wBase, CX_CHANCFG, szName); //    create a new chan cfg obj in current document
               if( wKey == CX_NULLOBJ_KEY ) 
               {  
                  bOk = FALSE; 
                  strMsg = _T("Low memory or document full");
               }
               else                                               //    import defn into this new chan cfg; if ok, 
               {                                                  //    update imported keys array and (name, key) map
                  CCxChannel* pChan =                             //    to include the imported object; if not, inform 
                     (CCxChannel*) pDoc->GetObject( wKey );       //    user and remove the new chan cfg from document.
                  if( pChan->Import( strArDef, strMsg ) )
                  {
                     m_wArKeys.Add( wKey );
                     m_importMap.SetAt( szName, wKey );
                     ++nImported;
                  }
                  else 
                  {
                     CString strTemp = _T("Failed to import chan cfg ");
                     strTemp += szName;
                     strTemp += _T(": ");
                     strTemp += strMsg;
                     pApp->LogMessage( strMsg );
                     pDoc->RemoveObj( wKey );
                  }
                  ++nRead;
               }
            }
         }
      }                                                           // END: import chan cfgs defined in file

      if( !bOk )                                                  // import aborted at some point -- report error:
      {
         if( nRead < 0 ) str = _T("Unexpected EOF or other error in header of channels.ini");
         else str.Format( "Error in block %d of channels.ini: %s", nRead, strMsg );
         pApp->LogMessage( str );
      }
   }
   catch( CFileException *e )                                     // import aborted on a file I/O exception
   {
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      str = _T("File I/O error on channels.ini: ");
      str += szErr;
      pApp->LogMessage( str );
   } 

   // close the file -- we'll ignore exceptions for now
   try { file.Close(); } catch (CFileException *e) { e->Delete(); }

   if( nImported > 0 )
   {
      str.Format( "%d channel configurations imported from channels.ini", nImported );
      pApp->LogMessage( str );
   }
}


//=== ImportPerts ===================================================================================================== 
//
//    If the import directory contains the file "perts.ini", import all perturbation object definitions (there could be 
//    more than one) in the file into new CCxPert objects under the CNTRLX "Perturbations" subtree.  Each perturbation 
//    that is successfully imported is registered in the array of imported keys and the import map. 
//
//    CntrlxUNIX perturbations were unnamed, identified only by their zero-based position in the perturbation list. 
//    Here we assign perturbation N the name "pertN" in the import map by design, since that is what CCxTrial::Import() 
//    will look for when it searches for a perturbation object in the map!
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportPerts()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString str = m_strDir + _T("\\perts.ini");                    // try to open pert defn file in current import dir 
   CStdioFile file;
   if( !file.Open( str, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      pApp->LogMessage( _T("Could not find or open perts.ini") );
      return;
   }

   BOOL bOk = TRUE; 

   int nPerts = 0;                                                // #perturbations defined in file
   int nRead = -1;                                                // #perturbations read from file thus far
   int nImported = 0;                                             // #perturbations successfully imported from file 
   CStringArray strArDef;                                         // set of text lines defining pert obj being imported 
   CString strName;                                               // name assigned to pert in import map: "pert<I>"
   CString strMsg;                                                // error message
   WORD wKey;

   WORD wBase = pDoc->GetBaseObj( CX_PERTBASE );                  // all imported perts are inserted under this obj 
   ASSERT( wBase != CX_NULLOBJ_KEY );

   try                                                            // (CStdioFile::ReadString throws CFileException)
   {
      // first line must be "NUMPERTS <N>", where N>0 is the #perturbations defined in the file
      bOk = file.ReadString( str );                               
      if(bOk) bOk = BOOL(1 == ::sscanf_s(str, "NUMPERTS %d", &nPerts) && (nPerts > 0));

      while( bOk && nRead < nPerts )                              // BEGIN: import perturbations defined in file...
      {
         if( nRead < 0 )                                          //    special case: read "PERTURBATION ..." line for 
         {                                                        //    the 1st defn block
            nRead = 0;
            bOk = file.ReadString( str );
            if( !bOk ) { strMsg = _T("Unexpected EOF"); break; }
         }

         int iPos = -1;                                           //    "PERTURBATION <i> ..." is expected format of 
         if(1 > ::sscanf_s( str, "PERTURBATION %d", &iPos) ||     //    1st line of defn block, where <i> is cardinal 
             iPos < 0 )                                           //    pos of pert in the cxUNIX pert list. in cxUNIX, 
         {                                                        //    perts were identified only by this cardinal 
            bOk = FALSE;                                          //    pos.  CCxTrial::Import() expects them to be 
            strMsg = _T("Unrecognized format");                   //    identified as "pert<I>" in the import map, 
         }                                                        //    where <I> is the cardinal pos.  we take care of 
         else                                                     //    that detail here. 
            strName.Format( "pert%d", iPos ); 

         strArDef.RemoveAll();                                    //    prepare string array defining pert...
         strArDef.Add( str );                                     //    this is the "PERTURBATION ..." line
         while( bOk )                                             //    now read in rest of defn block 
         {
            if( file.ReadString( str ) ) 
            {
               if( str.Find( _T("PERTURBATION") ) > -1 ) break;   //       we've reached start of next block!
               else strArDef.Add( str );
            }
            else                                                  //       we hit EOF while processing block...
            {
               bOk = BOOL(nRead >= nPerts-1);                     //       ...an error only if we're NOT on last block
               if( !bOk ) strMsg = _T("Unexpected EOF");
               break;
            }
         }

         if( bOk )                                                //    attempt import: 
         {
            if( m_importMap.Lookup( strName, wKey ) )             //    if cxUNIX name already in import map, then we 
            {                                                     //    cannot import the perturbation obj; however, we 
               strMsg = _T("Skipped over ");                      //    do not fail in this case -- we merely skip over 
               strMsg += strName;                                 //    this particular pert
               strMsg += _T("; name already imported!");
               pApp->LogMessage( strMsg );
               ++nRead;
            }
            else 
            {
               wKey= pDoc->InsertObj(wBase, CX_PERTURB, strName); //    create a new pert obj in current document
               if( wKey == CX_NULLOBJ_KEY ) 
               {  
                  bOk = FALSE; 
                  strMsg = _T("Low memory or document full");
               }
               else                                               //    import defn into this new pert obj; if ok, 
               {                                                  //    update imported keys array and (name, key) map
                  CCxPert* pPert =                                //    to include the imported object; if not, inform 
                     (CCxPert*) pDoc->GetObject( wKey );          //    user and remove the new pert obj from document. 
                  if( pPert->Import( strArDef, strMsg ) )         
                  {
                     m_wArKeys.Add( wKey );
                     m_importMap.SetAt( strName, wKey );
                     ++nImported;
                  }
                  else 
                  {
                     CString strTemp = _T("Failed to import ");
                     strTemp += strName;
                     strTemp += _T(": ");
                     strTemp += strMsg;
                     pApp->LogMessage( strMsg );
                     pDoc->RemoveObj( wKey );
                  }
                  ++nRead;
               }
            }
         }
      }                                                           // END: import perturbations defined in file

      if( !bOk )                                                  // import aborted at some point -- report error:
      {
         if( nRead < 0 ) str = _T("Unexpected EOF or other error in header of perts.ini");
         else str.Format( "Error in block %d of perts.ini: %s", nRead, strMsg );
         pApp->LogMessage( str );
      }
   }
   catch( CFileException *e )                                     // import aborted on a file I/O exception
   {
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      str = _T("File I/O error on perts.ini: ");
      str += szErr;
      pApp->LogMessage( str );
   } 

   // close the file -- we'll ignore exceptions for now 
   try { file.Close(); }
   catch (CFileException *e) { e->Delete(); }

   if( nImported > 0 )
   {
      str.Format( "%d perturbations imported from perts.ini", nImported );
      pApp->LogMessage( str );
   }
}


//=== ImportTargets =================================================================================================== 
//
//    If the import directory contains the folder "\targets", search the folder for both files and subfolders.  All 
//    cntrlxUNIX target definition files found in \targets are imported as new CCxTarget objects under the CNTRLX 
//    "Targets" subtree itself.  Each subfolder under \targets becomes a target set object with the same name as the 
//    subfolder, and target files in the subfolder are imported as CCxTarget objects under that set.  Each target 
//    object that is successfully imported is registered in the import map and in the array of imported keys.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportTargets()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages

   CFileFind fileFinder;                                          // for searching import dir for files & subdirs

   CString strSearch = m_strDir + _T("\\targets");                // first, is there a \targets subdir at all?
   BOOL bFound = fileFinder.FindFile( strSearch );
   if( bFound )                                                   // if found, make sure it's a directory!
   {
      fileFinder.FindNextFile();                                  // must invoke this to get info on file/dir found
      bFound = fileFinder.IsDirectory();
   }
   fileFinder.Close();                                            // reset file finder

   if( !bFound )                                                  // if no \targets subdir, then there are no target
   {                                                              // sets to import!
      pApp->LogMessage( _T("No \\targets subfolder in import dir!") );
      return;
   }

   ImportTargetSet( strSearch, NULL );                            // import any tgt files in \targets folder itself 

   strSearch += _T("\\*.*");                                      // search \targets folder for "target set" subfolders 
   bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( fileFinder.IsDirectory() && !fileFinder.IsDots() )      // for each subfolder found: import tgt files into a 
         ImportTargetSet( fileFinder.GetFilePath(),               // tgt set with same name as the subfolder
                          fileFinder.GetFileTitle() ); 
   }
}


//=== ImportTargetSet ================================================================================================= 
//
//    Create a target set of the specified name, then import all cntrlxUNIX target definition files in the specified 
//    folder as CCxTarget objects under that set.  No target set will be created if no target files are found or 
//    successfully imported. 
//
//    ARGS:       szFolder    -- [in] full pathname of directory to be searched for target definition files.
//                szSetName   -- [in] suggested name for tgt set object that will parent the imported tgt objects.  if
//                               NULL, then any imported tgts are placed in the root of the CNTRLX "Targets" subtree. 
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportTargetSet( LPCTSTR szFolder, LPCTSTR szSetName )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   WORD wBase = pDoc->GetBaseObj( CX_TARGBASE );                  // root of "Targets" subtree in the current document 
   ASSERT( wBase != CX_NULLOBJ_KEY );

   WORD wSet;                                                     // key of tgt set into which tgt files are imported 
   if( szSetName == NULL )                                        // in this case, import to root of CNTRLX tgt subtree 
      wSet = wBase;
   else                                                           // else, create new target set object w/specified 
   {
      wSet = pDoc->InsertObj( wBase, CX_TARGSET, szSetName );
      if( wSet == CX_NULLOBJ_KEY )                                // this is a catastrophic error
      {
         pApp->LogMessage( _T("Could not import target set:  low memory or document full") );
         return;
      }
   }

   int nTgts = 0;                                                 // #tgts imported from specified folder

   CFileFind fileFinder;                                          // search specified folder for files.  for each 
   CString strSearch = szFolder;                                  // folder found, read it as a cxUNIX target defn  
   strSearch += _T("\\*.*");                                      // file and attempt to import it...
   BOOL bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( !fileFinder.IsDirectory() )
      {
         if( ImportTargetFile( fileFinder.GetFilePath(), wSet ) ) ++nTgts;
      }
   }

   if( nTgts > 0 )                                                // report #tgts imported. if >0 and we created the 
   {                                                              // tgt set obj, be sure to add the set's key to the 
      CString strMsg;                                             // array of imported object keys...
      if( szSetName != NULL ) 
      { 
         m_wArKeys.Add( wSet );
         strMsg.Format( "Imported %d targets into set %s", nTgts, pDoc->GetObjName( wSet ) );
      }
      else 
         strMsg.Format( "Imported %d targets into target tree root", nTgts );
      pApp->LogMessage( strMsg );
   }
   else if( szSetName != NULL )                                   // if no tgt files imported and we created the tgt 
      pDoc->RemoveObj( wSet );                                    // set, then delete the (empty) set!
}


//=== ImportTargetFile ================================================================================================ 
//
//    Create a new CCxTarget object and import its definition from the specified file.  If successful, the new target 
//    object is registered in the import map and the array of imported keys.
//
//    ARGS:       szFile   -- [in] full pathname of target definition file.
//                wSet     -- [in] key of target set object which will parent the imported target object.
//
//    RETURNS:    TRUE if target file successfully imported; FALSE otherwise.
//
//    THROWS:     CMemoryException
//
BOOL CCxImporter::ImportTargetFile( LPCTSTR szFile, WORD wSet )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString strMsg;                                                // error message
   strMsg.Format( "Could not import target file %s: ", szFile );

   CStdioFile file;                                               // try to open specified file
   if( !file.Open( szFile, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      strMsg += _T("Failed to open file");
      pApp->LogMessage( strMsg );
      return( FALSE );
   }

   BOOL bOk = TRUE; 

   CStringArray strArDef;                                         // set of text lines defining tgt obj to import
   TCHAR szName[CX_MAXOBJNAMELEN+1];                              // cntrlxUNIX name of target being imported
   CString str;                                                   // for reading text file one line at a time

   try                                                            // (CStdioFile::ReadString throws CFileException)
   {
      bOk = file.ReadString( str );                               // first line must be "TARGET_FOR_CNTRLX86", which we 
      if( bOk ) bOk = BOOL(str == "TARGET_FOR_CNTRLX86");         // otherwise ignore

      if( bOk )                                                   // 2nd line is "VERSION <v>", which is otherwise 
      {                                                           // ignored
         bOk = file.ReadString( str );
         if( bOk ) bOk = BOOL(0 == ::strncmp( str, "VERSION", 7 ));
      }

      // 3rd line is "TARGETNAME <name>", where <name> is the cxUNIX name assigned to the target object. Here we read
      // the line from the file, parse it, ensure the name is not empty and does not exceed the max length.
      if(bOk)
      {
         bOk = file.ReadString(str) && BOOL(str.GetLength() <= CX_MAXOBJNAMELEN + 11) &&
            BOOL(1 == ::sscanf_s(str, "TARGETNAME %s", szName, CX_MAXOBJNAMELEN+1)) && BOOL(0 < ::strlen( szName ));
      }

      if( !bOk )                                                  // if an error occurred, then file fmt is wrong
         strMsg += _T("Bad header or target name");
      else 
      {                                                           // otherwise...
         WORD wKey;
         if( m_importMap.Lookup( szName, wKey ) )                 //    if cxUNIX name already in import map, then we 
         {                                                        //    cannot import the target obj 
            strMsg += _T("Target name already imported"); 
            bOk = FALSE;
         }
         else 
         {
            while( file.ReadString( str ) ) strArDef.Add( str );  //    read remaining text lines into string array

            wKey = pDoc->InsertObj( wSet, CX_XYTARG, szName );    //    create new tgt obj under specified tgt set;
            if( wKey == CX_NULLOBJ_KEY )                          //    assume its an XY scope tgt; CCxTarget::Import() 
            {                                                     //    can adjust this if necessary.
               strMsg += _T("Low memory or document full");
               bOk = FALSE; 
            }
            else                                                  //    import defn into the new tgt obj; if ok, update 
            {                                                     //    the imported keys array and (name,key) map to 
               CCxTarget* pTgt =                                  //    include the imported obj; if not, inform user 
                  (CCxTarget*) pDoc->GetObject( wKey );           //    and remove the new tgt obj from document. 
               if( pTgt->Import( strArDef, str ) ) 
               {
                  m_wArKeys.Add( wKey );
                  m_importMap.SetAt( szName, wKey );
               }
               else 
               {
                  strMsg += str;
                  bOk = FALSE;
                  pDoc->RemoveObj( wKey );
               }
            }
         }
      }
      
   }
   catch( CFileException *e )                                     // import aborted on a file I/O exception
   {
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      strMsg += _T("File I/O error - ");
      strMsg += szErr;
      bOk = FALSE;
   } 

   // close the file -- we'll ignore exceptions for now 
   try { file.Close(); }
   catch (CFileException *e) { e->Delete(); }

   if( !bOk ) pApp->LogMessage( strMsg );                         // report error if import failed
   return( bOk );
}


//=== ImportTrials ==================================================================================================== 
//
//    If the import directory contains the folder "\trials", search the folder for both files and subfolders.  All 
//    cntrlxUNIX trial definition files found in \trials are imported as new CCxTrial objects under a new trial set 
//    object with the suggested name "importSet" (may be modified to ensure uniqueness).  Each subfolder under \trials 
//    becomes a trial set object with the same name as the subfolder, and trial files in the subfolder are imported as 
//    CCxTrial objects under that set.  Each trial object that is successfully imported is registered in the array of 
//    imported keys.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportTrials()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages

   CFileFind fileFinder;                                          // for searching import dir for files & subdirs

   CString strSearch = m_strDir + _T("\\trials");                 // first, is there a \trials subdir at all?
   BOOL bFound = fileFinder.FindFile( strSearch );
   if( bFound )                                                   // if found, make sure it's a directory!
   {
      fileFinder.FindNextFile();                                  // must invoke this to get info on file/dir found
      bFound = fileFinder.IsDirectory();
   }
   fileFinder.Close();                                            // reset file finder

   if( !bFound )                                                  // if no \trials subdir, then there's nothing to do!
   { 
      pApp->LogMessage( _T("No \\trials subfolder in import dir!") );
      return;
   }

   ImportTrialSet( strSearch, NULL );                             // import any trial files in \trials folder itself 

   strSearch += _T("\\*.*");                                      // search \trials folder for "trial set" subfolders 
   bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( fileFinder.IsDirectory() && !fileFinder.IsDots() )      // for each subfolder found: import trial files into 
         ImportTrialSet( fileFinder.GetFilePath(),                // a trial set with same name as the subfolder
                         fileFinder.GetFileTitle() ); 
   }
}


//=== ImportTrialSet ================================================================================================== 
//
//    Create a target set of the specified name, then import all cntrlxUNIX target definition files in the specified 
//    folder as CCxTarget objects under that set.  No target set will be created if no target files are found or 
//    successfully imported. 
//
//    ARGS:       szFolder    -- [in] full pathname of directory to be searched for trial definition files.
//                szSetName   -- [in] suggested name for trial set object that will parent the imported trials.  if
//                               NULL, then we use the default name "importSet".
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportTrialSet( LPCTSTR szFolder, LPCTSTR szSetName )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   WORD wBase = pDoc->GetBaseObj( CX_TRIALBASE );                 // root of "Trials" subtree in the current document 
   ASSERT( wBase != CX_NULLOBJ_KEY );

   WORD wSet = pDoc->InsertObj( wBase, CX_TRIALSET,               // create trial set obj w/ name provided, or default  
            (szSetName == NULL) ? _T("importSet") : szSetName ); 
   if( wSet == CX_NULLOBJ_KEY )                                   // this is a catastrophic error
   {
      pApp->LogMessage( _T("Could not import trial set:  low memory or document full") );
      return;
   }

   int nTrials = 0;                                               // #trials imported from specified folder

   CFileFind fileFinder;                                          // search specified folder for files.  for each 
   CString strSearch = szFolder;                                  // folder found, read it as a cxUNIX trial defn  
   strSearch += _T("\\*.*");                                      // file and attempt to import it...
   BOOL bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( !fileFinder.IsDirectory() )
      {
         if( ImportTrialFile( fileFinder.GetFilePath(), wSet ) ) ++nTrials;
      }
   }

   if( nTrials > 0 )                                              // if #trials imported >0, report # and add the new 
   {                                                              // set's key to the array of imported object keys... 
      CString strMsg;                                             // array of imported object keys...
      m_wArKeys.Add( wSet );
      strMsg.Format( "Imported %d trials into set %s", nTrials, pDoc->GetObjName( wSet ) );
      pApp->LogMessage( strMsg );
   }
   else pDoc->RemoveObj( wSet );                                  // otherwise, delete the (empty) set!
}


//=== ImportTrialFile ================================================================================================= 
//
//    Create a new CCxTrial object and import its definition from the specified file.  If successful, the new trial is  
//    registered in the array of imported keys.  We also lock the trial's dependencies.
//
//    ARGS:       szFile   -- [in] full pathname of trial definition file.
//                wSet     -- [in] key of trial set object which will parent the imported trial.
//
//    RETURNS:    TRUE if trial file successfully imported; FALSE otherwise.
//
//    THROWS:     CMemoryException
//
BOOL CCxImporter::ImportTrialFile( LPCTSTR szFile, WORD wSet )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString strMsg;                                                // error message
   strMsg.Format( "Could not import trial file %s: ", szFile );

   CStdioFile file;                                               // try to open specified file
   if( !file.Open( szFile, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      strMsg += _T("Failed to open file");
      pApp->LogMessage( strMsg );
      return( FALSE );
   }

   BOOL bOk = TRUE; 

   CStringArray strArDef;                                         // set of text lines defining trial obj to import
   CString str;                                                   // for reading text file one line at a time

   try                                                            // (CStdioFile::ReadString throws CFileException)
   {
      while( file.ReadString( str ) ) strArDef.Add( str );        // read all text lines in file into the string array
   }
   catch( CFileException *e )                                     // import aborted on a file I/O exception
   {
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      strMsg += _T("File I/O error - ");
      strMsg += szErr;
      bOk = FALSE;
   } 

   // close the file -- we'll ignore exceptions for now 
   try { file.Close(); }
   catch (CFileException *e) { e->Delete(); }

   // get cxUNIX name of trial being imported from 3rd line of file: "TRIALNAME <name>". We parse that line to get the
   // trial name and verify it is neither an empty string nor exceeds the max allowed length
   TCHAR szName[CX_MAXOBJNAMELEN+1];
   if(bOk)
   { 
      bOk = BOOL(strArDef.GetSize() > 3) && BOOL(::strlen(strArDef[2]) <= CX_MAXOBJNAMELEN + 10) &&
         BOOL(1==::sscanf_s(strArDef[2],"TRIALNAME %s",szName, CX_MAXOBJNAMELEN+1)) && BOOL(0 < ::strlen(szName));
      if(!bOk) strMsg += _T("Unrecognized format");
   }

   if( bOk )                                                      // create new trial obj under specified trial set...
   {
      WORD wKey = pDoc->InsertObj( wSet, CX_TRIAL, szName );         
      if( wKey == CX_NULLOBJ_KEY ) 
      { 
         strMsg += _T("Low memory or document full");
         bOk = FALSE; 
      }
      else                                                        // ... then import defn into the new trial obj; if 
      {                                                           // ok, update the imported keys array to include the 
         CCxTrial* pTrial = (CCxTrial*) pDoc->GetObject( wKey );  // imported trial; if not, inform user & remove new 
         if( pTrial->Import( strArDef, m_importMap, str ) )       // trial obj from document. 
         {
            CWordArray wArEmpty;                                  // we also must update the trial's dependencies; when 
            pDoc->UpdateObjDep( wKey, wArEmpty );                 // first created, the dependency list is empty.
            m_wArKeys.Add( wKey );
         }
         else 
         {
            strMsg += str;
            bOk = FALSE;
            pDoc->RemoveObj( wKey );
         }
      }
   }

   if( !bOk ) pApp->LogMessage( strMsg );                         // report error if import failed
   return( bOk );
}


//=== ImportRuns ====================================================================================================== 
//
//    If the import directory contains the folder "\runs", search the folder for both files and subfolders.  All 
//    cntrlxUNIX run definition files found in \runs itself are imported as one or more CCxContRun objects under a new 
//    run set object with the suggested name "importSet" (may be modified to ensure uniqueness).  Each subfolder under 
//    the \runs folder becomes a run set object with the same name as the subfolder, and run files in the subfolder are 
//    imported under that set.  Each run set or stimulus run object that is successfully imported is registered in the 
//    array of imported keys.
//
//    ARGS:       NONE.
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportRuns()
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages

   CFileFind fileFinder;                                          // for searching import dir for files & subdirs

   CString strSearch = m_strDir + _T("\\runs");                   // first, is there a \runs subdir at all?
   BOOL bFound = fileFinder.FindFile( strSearch );
   if( bFound )                                                   // if found, make sure it's a directory!
   {
      fileFinder.FindNextFile();                                  // must invoke this to get info on file/dir found
      bFound = fileFinder.IsDirectory();
   }
   fileFinder.Close();                                            // reset file finder

   if( !bFound )                                                  // if no \runs subdir, then there's nothing to do!
   { 
      pApp->LogMessage( _T("No \\runs subfolder in import dir!") );
      return;
   }

   ImportRunSet( strSearch, NULL );                               // import any run files in \runs folder itself 

   strSearch += _T("\\*.*");                                      // search \runs folder for "run set" subfolders 
   bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( fileFinder.IsDirectory() && !fileFinder.IsDots() )      // for each subfolder found: import run files into 
         ImportRunSet( fileFinder.GetFilePath(),                  // a run set with same name as the subfolder
                         fileFinder.GetFileTitle() ); 
   }
}


//=== ImportRunSet ==================================================================================================== 
//
//    Create a stimulus run set of the specified name, then import all cntrlxUNIX stimulus run definition files in the 
//    specified folder as CCxContRun objects under that set.  Note that each run definition file may define one or more 
//    stimulus runs, each of which is imported as a separate CCxContRun object.  No run set will be created if no 
//    run definition files are found or no stimulus runs are successfully imported. 
//
//    ARGS:       szFolder    -- [in] full pathname of directory to be searched for run definition files.
//                szSetName   -- [in] suggested name for run set object that will parent the imported runs.  if NULL, 
//                               then we use the default name "importSet".
//
//    RETURNS:    NONE.
//
//    THROWS:     CMemoryException
//
VOID CCxImporter::ImportRunSet( LPCTSTR szFolder, LPCTSTR szSetName )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   WORD wBase = pDoc->GetBaseObj( CX_CONTRUNBASE );               // root of "Stimulus Runs" subtree in the current doc 
   ASSERT( wBase != CX_NULLOBJ_KEY );

   WORD wSet = pDoc->InsertObj( wBase, CX_CONTRUNSET,             // create run set obj w/ name provided, or default  
            (szSetName == NULL) ? _T("importSet") : szSetName ); 
   if( wSet == CX_NULLOBJ_KEY )                                   // this is a catastrophic error
   {
      pApp->LogMessage( _T("Could not import run set:  low memory or document full") );
      return;
   }

   int nRuns = 0;                                                 // #stimulus runs imported from specified folder

   CFileFind fileFinder;                                          // search specified folder for files.  for each 
   CString strSearch = szFolder;                                  // folder found, read it as a cxUNIX stimulus run 
   strSearch += _T("\\*.*");                                      // defn file and attempt to import it...
   BOOL bFound = fileFinder.FindFile( strSearch );
   while( bFound )
   {
      bFound = fileFinder.FindNextFile();
      if( !fileFinder.IsDirectory() )
      {
         int n = ImportStimRunFile( fileFinder.GetFilePath(), fileFinder.GetFileTitle(), wSet );
         if( n > 0 )
         {
            CString strMsg;
            strMsg.Format( "Imported %d stimulus runs from file %s into set %s",
                           n, fileFinder.GetFileTitle(), pDoc->GetObjName( wSet ) );
            pApp->LogMessage( strMsg );
            nRuns += n;
         }
      }
   }

   if( nRuns > 0 )                                                // if #runs imported >0, add the new set's key to the 
      m_wArKeys.Add( wSet );                                      // array of imported object keys... 
   else                                                           // otherwise, delete the (empty) set!
      pDoc->RemoveObj( wSet );
}




//=== ImportStimRunFile ================================================================================================ 
//
//    Import each stimulus run defined in the specified cntrlxUNIX stimulus run definition file as a new CCxContRun 
//    object contained in the specified run set object.  Each successfully imported stimulus run is registered in the 
//    array of imported keys.
//
//    ARGS:       szFile   -- [in] full pathname of the stimulus run definition file.
//                szFName  -- [in] the base filename of the stimulus run definition file.  Used to name stimulus run 
//                            object when the cxUNIX run file is older version lacking run name entry ("XSRUN <name>").
//                wSet     -- [in] key of run set object which will parent each imported stimulus run.
//
//    RETURNS:    number of stimulus runs that were successfully imported.
//
//    THROWS:     CMemoryException
//
int CCxImporter::ImportStimRunFile( LPCTSTR szFile, LPCTSTR szFName, WORD wSet )
{
   CCntrlxApp* pApp = (CCntrlxApp*) ::AfxGetApp();                // for logging progress, error messages
   CCxDoc* pDoc = pApp->GetDoc();                                 // the current CNTRLX experiment document (SDI!)

   CString strMsg;                                                // error message

   CStdioFile file;                                               // try to open specified file
   if( !file.Open( szFile, CFile::modeRead | CFile::shareExclusive | CFile::typeText ) )
   {
      strMsg.Format( "Could not import run file %s: Failed to open file", szFName );
      pApp->LogMessage( strMsg );
      return( 0 );
   }

   CStringArray strArFile;                                        // all text lines read in from the file
   CString str;

   try                                                            // (CStdioFile::ReadString throws CFileException)
   {
      while( file.ReadString( str ) ) strArFile.Add( str );       // read all text lines in file into a string array
   }
   catch( CFileException *e )                                     // import aborted on a file I/O exception
   {
      TCHAR szErr[100];
      e->GetErrorMessage( szErr, 100 );
      strMsg += _T("File I/O error (");
      strMsg += szErr;
      strMsg += _T(") on run set file ");
      strMsg += szFile;
      strArFile.RemoveAll();
   } 

   // close the file -- we ignore exceptions on closing
   try { file.Close(); }
   catch (CFileException *e) { e->Delete(); }

   if( strArFile.GetSize() == 0 )                                 // abort if a file I/O exception occurred!
   {
      pApp->LogMessage( strMsg );
      return( 0 );
   }

   int nImported = 0;                                             // #stimulus runs successfully imported

   if( ::strncmp( strArFile[0], "NUMSTIMULI", 10 ) == 0 )         // import old-style cxUNIX run file, which defines 
   {                                                              // a single stimulus run:
      WORD wKey = pDoc->InsertObj( wSet, CX_CONTRUN, szFName );   //    create new run obj under specified run set, 
      if( wKey == CX_NULLOBJ_KEY )                                //    using file's basename as name of run.
      { 
         strMsg.Format( "Aborted import of run file %s: Low memory or document full", szFName );
         pApp->LogMessage( strMsg );
      }
      else                                                        //    ... then import defn into the new run obj; if 
      {                                                           //    ok, update imported keys array to include the 
         CString strErr;                                          //    imported run; if not, inform user & remove new 
         CCxContRun* p = (CCxContRun*) pDoc->GetObject(wKey);     //    run obj from document. 
         if( p->Import( strArFile, strErr ) ) 
         {
            m_wArKeys.Add( wKey );
            nImported = 1;
         }
         else 
         {
            pDoc->RemoveObj( wKey );
            strMsg.Format( "Skipped run 0 in %s: %s", szFName, strErr );
            pApp->LogMessage( strMsg );
         }
      }
   }
   else                                                           // import cxUNIX run files that define one or more 
   {                                                              // stimulus runs, with run names included:
      int nRuns = 0;                                              //    #stimulus runs read from the file
      CStringArray strArDef;                                      //    text lines defining one stim run in the file
      TCHAR szRunName[CX_MAXOBJNAMELEN+1];                        //    name of a stimulus run defined in the file

      int i = 0; 
      while( i < strArFile.GetSize() )
      {
         while( i < strArFile.GetSize() &&                        //    skip to start of defn for next stim run in file 
                0 != ::strncmp( strArFile[i], "XSRUN", 5 ) )
           ++i;

         if( i >= strArFile.GetSize() ) break;                    //    reached EOF before start of a new defn

         strArDef.RemoveAll();                                    //    prepare array of text lines defining the next 
         while(i<strArFile.GetSize() && !strArFile[i].IsEmpty())  //    stimulus run in the run set file -- terminated 
         {                                                        //    by a blank line
            strArDef.Add( strArFile[i] );
            ++i;
         }
         ++nRuns;

         // get name of stim run from 1st line of block "XSRUN <name>": Verify the line is there, parse it, and ensure
         // the run name is neither empty nor too long. Skip block if line invalid.
         if((strArDef.GetSize() == 0) || (::strlen(strArDef[0]) > CX_MAXOBJNAMELEN + 6) ||
             (1 != ::sscanf_s(strArDef[0], "XSRUN %s", szRunName, CX_MAXOBJNAMELEN+1)) || (0 >= ::strlen(szRunName))) 
         { 
            strMsg.Format( "Skipped over run #%d in %s: Unrecognized format", nRuns-1, szFName );
            pApp->LogMessage( strMsg );
            continue;
         }

         WORD wKey= pDoc->InsertObj(wSet, CX_CONTRUN, szRunName); //    create new run obj under specified run set...
         if( wKey == CX_NULLOBJ_KEY ) 
         { 
            strMsg.Format( "Aborted import of run file %s: Low memory or document full", szFName );
            pApp->LogMessage( strMsg );
            i = static_cast<int>(strArFile.GetSize()); 
         }
         else                                                     //    ... then import defn into the new run obj; if 
         {                                                        //    ok, update imported keys array to include the 
            CString strErr;                                       //    imported run; if not, inform user & remove new 
            CCxContRun* p = (CCxContRun*) pDoc->GetObject(wKey);  //    run obj from document. 
            if( p->Import( strArDef, strErr ) ) 
            {
               m_wArKeys.Add( wKey );
               ++nImported;
            }
            else 
            {
               pDoc->RemoveObj( wKey );
               strMsg.Format( "Skipped over run #%d in %s: %s", nRuns-1, szFName, strErr );
               pApp->LogMessage( strMsg );
            }
         }
      }
   }

   return( nImported );
}


