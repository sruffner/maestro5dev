//===================================================================================================================== 
//
// cximporter.h : Declaration of class CCxImporter.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(CXIMPORTER_H__INCLUDED_)
#define CXIMPORTER_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


//===================================================================================================================== 
// Declaration of class CCxImporter  
//===================================================================================================================== 
//
class CCxImporter
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   CString        m_strDir;                           // full pathname of the import directory
   CWordArray     m_wArKeys;                          // keys of objects imported thus far
   CMap<CString, LPCTSTR, WORD, WORD>  m_importMap;   // cxUNIX names to keys of imported "independent" objects
   CCxSettings    m_importedSettings;                 // imported application settings 


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CCxImporter( const CCxImporter& src );                // copy constructor is NOT defined
   CCxImporter& operator=( const CCxImporter& src );     // assignment op is NOT defined

public:
   CCxImporter() {}                                      // constructor 
   virtual ~CCxImporter() {}                             // destructor 


//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 
public:
   VOID DoImport( LPCTSTR szDir );                       // import cntrlxUNIX obj defn files from a specified directory 


//===================================================================================================================== 
// IMPLEMENTATION  
//===================================================================================================================== 
private:
   BOOL ImportSettings();                                // helper methods for DoImport()
   VOID ImportChannels();
   VOID ImportPerts();
   VOID ImportTargets();
   VOID ImportTargetSet( LPCTSTR szFolder, LPCTSTR szSetName );
   BOOL ImportTargetFile( LPCTSTR szFile, WORD wSet );
   VOID ImportTrials();
   VOID ImportTrialSet( LPCTSTR szFolder, LPCTSTR szSetName );
   BOOL ImportTrialFile( LPCTSTR szFile, WORD wSet );
   VOID ImportRuns();
   VOID ImportRunSet( LPCTSTR szFolder, LPCTSTR szSetName );
   int ImportStimRunFile( LPCTSTR szFile, LPCTSTR szFName, WORD wSet );

};

#endif // !defined(CXIMPORTER_H__INCLUDED_)


