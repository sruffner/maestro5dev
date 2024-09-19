////////////////////////////////////////////////////////////////
// Microsoft Systems Journal -- October 1999
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
#pragma once
#ifndef NODIB
#include "dib.h"
#endif

//////////////////////////////////////////////////////////////////
// Splash screen. To use it, write:
//
// CSplash *pSplash = new CSplash(
//		IDB_MYBITMAP,			// resource ID of bitmap
//		duration,				// min time to display, in msec
//		flags,					// see below
//		&pSplash);				// address of back pointer
//
// If you want to kill the screen, you can call
//
// if (pSplash)
//		pSplash->Kill();
//
// but this is usually unecessary. You don't have to call delete either;
// CSplash will delete itself. When it does, it sets your pointer to NULL so
// you won't try to call Kill on a bad pointer.
//
class CSplash : public CWinThread {
public:
	CSplash(UINT nIDRes,				// resource ID of bitmap
		UINT duration,					// how long to show (minimum)
		WORD flags=0,					// see below
		CSplash** ppBackPtr=NULL);	// pointer to NULL when destroyed
	~CSplash();

	enum { // flags
		KillOnClick = 0x0001,		// any key/mouse dismisses splash
		IgnoreCmdLine = 0x0002,		// need I say more?
		NoWaitForMainWnd = 0x0004,	// don't wait for main window to expire
	};

	// override to create a different kind of splash window
	virtual CWnd* OnCreateSplashWnd(UINT nIDRes, UINT duration, WORD flags);
	void Kill();						// kill the splash screen

protected:
	CSplash**	m_ppBackPtr;		// caller's back pointer to me
	UINT			m_nIDRes;			// bitmap resource ID
	UINT			m_duration;			// how long to display
	WORD			m_flags;				// CSplashWnd creation flags

	virtual BOOL InitInstance();
	DECLARE_DYNAMIC(CSplash)
};

//////////////////
// Splash window. This class is private to CSplash--Don't use it unless
// you are doing some hairy stuff to override the splash window, like
// create animated effects, etc.
// 
class CSplashWnd : public CWnd {
protected:
	friend CSplash;
	CSplashWnd();
	~CSplashWnd();

#ifdef NODIB
	CBitmap	m_bitmap;		// ordinary MFC bitmap
#else
	CDib		m_dib;			// Device independent bitmap
#endif
	UINT		m_duration;		// duration (msec)
	WORD		m_flags;			// see below

	// override to do weird stuff
	virtual BOOL Create(UINT nIDRes, UINT duration, WORD flags);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();

	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CSplashWnd)
};


