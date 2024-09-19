////////////////////////////////////////////////////////////////
// Microsoft Systems Journal -- October 1999
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
// 31aug2017(sar) -- Fix warnings/errors during 64-bit compilation in VS 2017
//
#include "stdafx.h"
#include "splash.h"
#include "cmdline.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSplashWnd, CWnd)
BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////
// CSplashWnd : the splash window

CSplashWnd::CSplashWnd()
{
}

CSplashWnd::~CSplashWnd()
{
}

//////////////////
// Create splash window: load bitmap and create the window
//
BOOL CSplashWnd::Create(UINT nIDRes, UINT duration, WORD flags)
{
#ifdef NODIB
	// Code for ordinary bitmap (assumes m_bitmap is a CBitmap)
	if (!m_bitmap.LoadBitmap(nIDRes))
		return FALSE;
	BITMAP bm;
	m_bitmap.GetBitmap(&bm);
	CSize sz(bm.bmWidth, bm.bmHeight);
#else
	if (!m_dib.Load(nIDRes))
		return FALSE;
	CSize sz = m_dib.GetSize();
#endif

	m_duration = duration;
	m_flags = flags;
	return CreateEx(0,
		AfxRegisterWndClass(0, AfxGetApp()->LoadStandardCursor(IDC_ARROW)),
		NULL,
		WS_POPUP | WS_VISIBLE,
		0, 0, sz.cx, sz.cy,
		NULL, // parent wnd
		NULL);
}

//////////////////
// Splash window created: center it, move to foreground and set a timer
//
int CSplashWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CenterWindow();
	UpdateWindow();
	SetForegroundWindow();
	if (m_duration!=-1)
		SetTimer(1, m_duration, NULL);
	return 0;
}

//////////////////
// The window has been destroyed: put main app in foreground
// and post a message to quit this thread.
//
void CSplashWnd::PostNcDestroy()
{
	CWinApp* pApp = AfxGetApp();
	CWnd* pMainWnd = pApp->m_pMainWnd;
	if (IsWindow(pMainWnd->GetSafeHwnd()))
		::SetForegroundWindow(pMainWnd->GetSafeHwnd());
	delete this;
}

//////////////////
// Draw the bitmap.
//
void CSplashWnd::OnPaint()
{
	CPaintDC dc(this);
#ifdef NODIB
	// Code for ordinary bitmap:
	CDC dcImage;
	if (!dcImage.CreateCompatibleDC(&dc))
		return;
	BITMAP bm;
	m_bitmap.GetBitmap(&bm);

	// Paint the image.
	CBitmap* pOldBitmap = dcImage.SelectObject(&m_bitmap);
	dc.BitBlt(0, 0, bm.bmWidth, bm.bmHeight, &dcImage, 0, 0, SRCCOPY);
	dcImage.SelectObject(pOldBitmap);
#else
	m_dib.Draw(dc);
#endif
}

//////////////////
// Timer expired: kill myself--unless the app has
// not created a main window yet.
//
void CSplashWnd::OnTimer(UINT_PTR nIDEvent)
{
	CWinApp* pApp = AfxGetApp();
	CWnd* pMainWnd = pApp->m_pMainWnd;
	if ((m_flags & CSplash::NoWaitForMainWnd) ||
		IsWindow(pMainWnd->GetSafeHwnd()))
		// have main window: OK to die
		SendMessage(WM_CLOSE);
	else
		// no main window: keep splashing
		SetTimer(1,100,NULL);
}

//////////////////
// Before translating keystroke or mouse: die
//
BOOL CSplashWnd::PreTranslateMessage(MSG* pMsg)
{
	if (m_flags & CSplash::KillOnClick) {
		UINT msg = pMsg->message;
		if (msg == WM_KEYDOWN ||
			 msg == WM_SYSKEYDOWN ||
			 msg == WM_LBUTTONDOWN ||
			 msg == WM_RBUTTONDOWN ||
			 msg == WM_MBUTTONDOWN)
		{
			PostMessage(WM_CLOSE); // post don't send, to let current msg process
			return TRUE;			  // (eat current message)
		}
	}
	return CWnd::PreTranslateMessage(pMsg);
}

////////////////////////////////////////////////////////////////
// CSplash, a thread object

IMPLEMENT_DYNAMIC(CSplash, CWinThread)

//////////////////
// Create a new splash thread
//
CSplash::CSplash(UINT nIDRes, UINT duration, WORD flags, CSplash** ppBackPtr)
{
	m_ppBackPtr = ppBackPtr;
	m_nIDRes = nIDRes;
	m_duration = duration;
	m_flags = flags;
	CreateThread();
}

/////////////////
// Destruction: Set caller's pointer to NULL, so he knows I'm gone.
//
CSplash::~CSplash()
{
	if (m_ppBackPtr)
		*m_ppBackPtr = NULL;
}

//////////////////
// Thread initialization.
// Returns TRUE to keep running, otherwise FALSE
// if I determine I'm not supposed to run the splash
//
BOOL CSplash::InitInstance()
{
	// Check for -nologo switch
	CWinApp* pApp = AfxGetApp();
	ASSERT(pApp);

	// Look for -nologo switch, or any others that MFC thinks should
	// prohibit a splash screen (such as OLE embedding, etc.)
	//
	if (!(m_flags & IgnoreCmdLine)) {
		CCommandLineInfoEx cmdinfo;
		pApp->ParseCommandLine(cmdinfo);
		if (!cmdinfo.m_bShowSplash || cmdinfo.GetOption(_T("nologo")))
			return FALSE;
	}
	if (!AfxOleGetUserCtrl())  // running without UI: to be safe
		return FALSE;

	// Create the splash window
	m_pMainWnd = OnCreateSplashWnd(m_nIDRes, m_duration, m_flags);
	return m_pMainWnd != NULL;
}

//////////////////
// Create the splash window. This is virtual so you can override to create
// somet other kind of window if you like. 
//
CWnd* CSplash::OnCreateSplashWnd(UINT nIDRes, UINT duration, WORD flags)
{
	CSplashWnd *pSplashWnd = new CSplashWnd;
	if (pSplashWnd)
		pSplashWnd->Create(nIDRes, duration, flags);
	return pSplashWnd;
}

//////////////////
// Kill the splash window. Could set a CEvent to
// terminate thread, but easier to post a close message.
//
void CSplash::Kill()
{
	if (m_pMainWnd)
		m_pMainWnd->PostMessage(WM_CLOSE);
}
