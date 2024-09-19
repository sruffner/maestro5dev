//===================================================================================================================== 
//
// CxAbout.cpp : Implementation of class CCxAbout, a simple modal "About" dialog box for the CNTRLX application.
//
// Author:  saruffner
//
// Description:
//
// Revision History:
//
// 10feb2000-- Created.  Very simple class derived from CDialog.  This can serve as a base for something more 
//             interesting in the future.
//
//===================================================================================================================== 


#include "stdafx.h"
#include "cntrlx.h"

#include "cxabout.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP( CCxAbout, CDialog )
END_MESSAGE_MAP()


//===================================================================================================================== 
// OPERATIONS -- CONSTRUCTION/DESTRUCTION
//===================================================================================================================== 

// our constructor uses CDialog's native constructor, but initializes it with the dialog template we want to use
//
CCxAbout::CCxAbout() : CDialog( CCxAbout::IDD )
{
}


//===================================================================================================================== 
// OPERATIONS -- MESSAGE MAP HANDLERS
//===================================================================================================================== 


//===================================================================================================================== 
// OPERATIONS -- ALL OTHER
//===================================================================================================================== 

void CCxAbout::DoDataExchange
   (
   CDataExchange* pDX
   )
{
   CDialog::DoDataExchange( pDX );
}


