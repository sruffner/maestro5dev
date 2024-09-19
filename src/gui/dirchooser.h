//===================================================================================================================== 
//
// dirchooser.h : Declaration of class CDirChooser.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//===================================================================================================================== 


#if !defined(DIRCHOOSER_H__INCLUDED_)
#define DIRCHOOSER_H__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000



//===================================================================================================================== 
// Declaration of class CDirChooser  
//===================================================================================================================== 
//
class CDirChooser
{
//===================================================================================================================== 
// DATA OBJECTS
//===================================================================================================================== 
private: 
   CString     m_strDir;                                 // full pathname for the currently chosen directory


//===================================================================================================================== 
// CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 
private:
   CDirChooser( const CDirChooser& src );                // copy constructor is NOT defined
   CDirChooser& operator=( const CDirChooser& src );     // assignment op is NOT defined

public:
   CDirChooser();                                        // constructor 
   virtual ~CDirChooser() {}                             // destructor 


//===================================================================================================================== 
// OPERATIONS  
//===================================================================================================================== 
public:
   BOOL Browse( HWND hWndOwner, LPCTSTR szTitle,         // initiate dialog allowing user to browse for a directory
                LPCTSTR szInitDir = NULL );
   const CString& GetChosenDirectory() const             // retrieve the chosen directory
   { 
      return( m_strDir ); 
   }


//===================================================================================================================== 
// IMPLEMENTATION  
//===================================================================================================================== 
private:
   static int CALLBACK BrowseCallback(                   // callback selects initial dir after browse dialog init'd 
      HWND hWnd, UINT nMsg, LPARAM lParam, LPARAM lpData );

};

#endif // !defined(DIRCHOOSER_H__INCLUDED_)


