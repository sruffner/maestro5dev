////////////////////////////////////////////////////////////////////////////
// TitleTip.cpp : implementation file
//
// Based on code by Zafir Anjum
//
// Adapted by Chris Maunder <cmaunder@mail.com>
// Copyright (c) 1998-2000. All Rights Reserved.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. 
//
// An email letting me know how you are using it would be nice as well. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// For use with CGridCtrl v2.20+
//
// History
//         10 Apr 1999  Now accepts a LOGFONT pointer and 
//                      a tracking rect in Show(...)  (Chris Maunder)
//         18 Apr 1999  Resource leak in Show fixed by Daniel Gehriger
//          8 Mar 2000  Added double-click fix found on codeguru
//                      web site but forgot / can't find who contributed it
//         28 Mar 2000  Aqiruse (marked with //FNA)
//                      Titletips now use cell color
//         18 Jun 2000  Delayed window creation added
//
// ----------------------------------
// UNOFFICIAL MODS (SAR):
// 05dec2000-- Added flag bShowAlways to Show() -- a mechanism by which to 
//             show title tips regardless of whether the specified text fits 
//             into the specified rect...
//          -- So that parent windows can detect right clicks on a title tip, 
//             modified PreTranslateMessage() to pass on WM_RBUTTONUP as well 
//             as WM_RBUTTONDOWN.
// 25nov2002-- Added argument iLifetime to Show().  If this is set to a positive 
//             value N, we use a one-shot timer to extinguish the tip after N 
//             ms have elapsed -- instead of relying on mouse capture.  This 
//             allows one to show the tip even if the mouse is not hovering 
//             in a particular area...  The timer interval is restricted to 
//             [200..1000] milliseconds.
/////////////////////////////////////////////////////////////////////////////
 
#include "stdafx.h"
#include "gridctrl.h"

#ifndef GRIDCONTROL_NO_TITLETIPS

#include "titletip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTitleTip

CTitleTip::CTitleTip()
{
   // Register the window class if it has not already been registered.
   WNDCLASS wndcls;
   HINSTANCE hInst = AfxGetInstanceHandle();
   if(!(::GetClassInfo(hInst, TITLETIP_CLASSNAME, &wndcls)))
   {
      // otherwise we need to register a new class
      wndcls.style         = CS_SAVEBITS;
      wndcls.lpfnWndProc      = ::DefWindowProc;
      wndcls.cbClsExtra      = wndcls.cbWndExtra = 0;
      wndcls.hInstance      = hInst;
      wndcls.hIcon         = NULL;
      wndcls.hCursor         = LoadCursor( hInst, IDC_ARROW );
      wndcls.hbrBackground   = (HBRUSH)(COLOR_INFOBK +1);
      wndcls.lpszMenuName      = NULL;
      wndcls.lpszClassName   = TITLETIP_CLASSNAME;

      if (!AfxRegisterClass(&wndcls))
         AfxThrowResourceException();
   }

    m_dwLastLButtonDown = ULONG_MAX;
    m_dwDblClickMsecs   = GetDoubleClickTime();
    m_bCreated          = FALSE;
    m_nTimerID          = 0;
}

CTitleTip::~CTitleTip()
{
}


BEGIN_MESSAGE_MAP(CTitleTip, CWnd)
   //{{AFX_MSG_MAP(CTitleTip)
   ON_WM_MOUSEMOVE()
   ON_WM_TIMER()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTitleTip message handlers

BOOL CTitleTip::Create(CWnd * pParentWnd)
{
   ASSERT_VALID(pParentWnd);

    // Already created?
    if (m_bCreated)
        return TRUE;

   DWORD dwStyle = WS_BORDER | WS_POPUP; 
   DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
   m_pParentWnd = pParentWnd;

   m_bCreated = CreateEx(dwExStyle, TITLETIP_CLASSNAME, NULL, dwStyle, 
                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                        NULL, NULL, NULL );

    return m_bCreated;
}

BOOL CTitleTip::DestroyWindow() 
{
   if( m_nTimerID != 0 ) { KillTimer( m_nTimerID ); m_nTimerID = 0; }         // 25nov2002(SAR)
   m_bCreated = FALSE;
   return CWnd::DestroyWindow();
}

// Show        - Show the titletip if needed
// rectTitle    - The rectangle within which the original 
//                title is constrained - in client coordinates
// lpszTitleText - The text to be displayed
// xoffset       - Number of pixel that the text is offset from
//               left border of the cell
//
// 05dec2000(SAR)-- Added arg bShowAlways to show a title tip even if the specified 
//    text fits inside the specified rectangle.  Defaults to FALSE.
// 25nov2002(SAR)-- Added arg nTimerMS to extinguish title tip after a set interval as an alternative to basing it on 
//    the captured mouse position relative to a hover rect.
void CTitleTip::Show(CRect rectTitle, LPCTSTR lpszTitleText, int xoffset /*=0*/,
                     LPRECT lpHoverRect /*=NULL*/,
                     const LOGFONT* lpLogFont /*=NULL*/,
                     COLORREF crTextClr /* CLR_DEFAULT */,
                     COLORREF crBackClr /* CLR_DEFAULT */, 
                     BOOL bShowAlways /* FALSE */,
                     int nTimerMS /* = -1 */ )
{
    if (!IsWindow(m_hWnd))
        Create(m_pParentWnd);

   ASSERT( ::IsWindow( GetSafeHwnd() ) );

    if (rectTitle.IsRectEmpty())
        return;

   // If titletip is already displayed, don't do anything.
   if( IsWindowVisible() ) 
      return;

    m_rectHover = (lpHoverRect != NULL)? lpHoverRect : rectTitle;
    m_rectHover.right++; m_rectHover.bottom++;

   m_pParentWnd->ClientToScreen( m_rectHover );
    ScreenToClient( m_rectHover );

   // Do not display the titletip is app does not have focus
   if( GetFocus() == NULL )
      return;

   // Define the rectangle outside which the titletip will be hidden.
   // We add a buffer of one pixel around the rectangle
   m_rectTitle.top    = -1;
   m_rectTitle.left   = -xoffset-1;
   m_rectTitle.right  = rectTitle.Width()-xoffset;
   m_rectTitle.bottom = rectTitle.Height()+1;

   // Determine the width of the text
   m_pParentWnd->ClientToScreen( rectTitle );

   CClientDC dc(this);
   CString strTitle = _T("");
    strTitle += _T(" ");
    strTitle += lpszTitleText; 
    strTitle += _T(" ");

   CFont font, *pOldFont = NULL;
    if (lpLogFont)
    {
        font.CreateFontIndirect(lpLogFont);
       pOldFont = dc.SelectObject( &font );
    }
    else
    {
        // use same font as ctrl
       pOldFont = dc.SelectObject( m_pParentWnd->GetFont() );
    }

   CSize size = dc.GetTextExtent( strTitle );

    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    size.cx += tm.tmOverhang;

   CRect rectDisplay = rectTitle;
   rectDisplay.left += xoffset;
   rectDisplay.right = rectDisplay.left + size.cx + xoffset;
    
    // Do not display if the text fits within available space, unless 
    // requested to show tip regardless...
    if ( bShowAlways || (rectDisplay.right > rectTitle.right-xoffset) )
    {
        // Show the titletip
        SetWindowPos( &wndTop, rectDisplay.left, rectDisplay.top, 
            rectDisplay.Width(), rectDisplay.Height(), 
            SWP_SHOWWINDOW|SWP_NOACTIVATE );
        
        // FNA - handle colors correctly
        if (crBackClr != CLR_DEFAULT)
        {
          CBrush backBrush(crBackClr);
          CBrush* pOldBrush = dc.SelectObject(&backBrush);
          CRect rect;
          dc.GetClipBox(&rect);     // Erase the area needed 

          dc.PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),  PATCOPY);
          dc.SelectObject(pOldBrush);
       }
        // Set color
        if (crTextClr != CLR_DEFAULT)//FNA
            dc.SetTextColor(crTextClr);//FA

        dc.SetBkMode( TRANSPARENT );
        dc.TextOut( 0, 0, strTitle );

         if( nTimerMS <= 0 )           // the default behavior:  extinguish when mouse leaves hover rect
            SetCapture();
         else                          // alternative behavior:  extinguish after a one-shot timer
         {
            nTimerMS = (nTimerMS < 200) ? 200 : ((nTimerMS > 1000) ? 1000 : nTimerMS);
            m_nTimerID = SetTimer( WM_SETFOCUS, nTimerMS, NULL );
            if( m_nTimerID == 0 ) Hide();
         }
    }
    
    dc.SelectObject( pOldFont );
}

void CTitleTip::Hide()
{
   if( !::IsWindow(GetSafeHwnd()) ) return;
   if( GetCapture()->GetSafeHwnd() == GetSafeHwnd() )
      ReleaseCapture();
   if( m_nTimerID != 0 )
   {
      KillTimer( m_nTimerID );
      m_nTimerID = 0;
   }
   ShowWindow( SW_HIDE );
}

void CTitleTip::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (!m_rectHover.PtInRect(point)) 
    {
        Hide();
        
        // Forward the message
        ClientToScreen( &point );
        CWnd *pWnd = WindowFromPoint( point );
        if ( pWnd == this ) 
            pWnd = m_pParentWnd;
        
        int hittest = (int)pWnd->SendMessage(WM_NCHITTEST,0,MAKELONG(point.x,point.y));
        
        if (hittest == HTCLIENT) {
            pWnd->ScreenToClient( &point );
            pWnd->PostMessage( WM_MOUSEMOVE, nFlags, MAKELONG(point.x,point.y) );
        } else {
            pWnd->PostMessage( WM_NCMOUSEMOVE, hittest, MAKELONG(point.x,point.y) );
        }
    }
}

// 25nov2002(SAR)-- Extinguish title tip on expiration of "one-shot" timer.
// 31aug2017(SAR)-- Changed arg to UINT_PTR for 64-bit compilation.
void CTitleTip::OnTimer( UINT_PTR nIDEvent ) 
{
   if( nIDEvent != WM_SETFOCUS ) return;
   Hide();
}


BOOL CTitleTip::PreTranslateMessage(MSG* pMsg) 
{
    // Used to qualify WM_LBUTTONDOWN messages as double-clicks
    DWORD dwTick=0;
    BOOL bDoubleClick=FALSE;

    CWnd *pWnd;
   int hittest;
   switch (pMsg->message)
   {
   case WM_LBUTTONDOWN:
       // Get tick count since last LButtonDown
        dwTick = GetTickCount();
        bDoubleClick = ((dwTick - m_dwLastLButtonDown) <= m_dwDblClickMsecs);
        m_dwLastLButtonDown = dwTick;
        // NOTE: DO NOT ADD break; STATEMENT HERE! Let code fall through

   case WM_RBUTTONDOWN:
   case WM_RBUTTONUP: 
   case WM_MBUTTONDOWN:
	   {
          POINTS pts = MAKEPOINTS( pMsg->lParam );
          POINT  point;
          point.x = pts.x;
          point.y = pts.y;
          ClientToScreen( &point );
          pWnd = WindowFromPoint( point );
          if( pWnd == this ) 
             pWnd = m_pParentWnd;

          hittest = (int)pWnd->SendMessage(WM_NCHITTEST,0,MAKELONG(point.x,point.y));

          if (hittest == HTCLIENT) {
             pWnd->ScreenToClient( &point );
             pMsg->lParam = MAKELONG(point.x,point.y);
          } else {
             switch (pMsg->message) {
                case WM_LBUTTONDOWN: 
                   pMsg->message = WM_NCLBUTTONDOWN;
                   break;
                case WM_RBUTTONDOWN: 
                   pMsg->message = WM_NCRBUTTONDOWN;
                   break;
                case WM_MBUTTONDOWN: 
                   pMsg->message = WM_NCMBUTTONDOWN;
                   break;
             }
             pMsg->wParam = hittest;
             pMsg->lParam = MAKELONG(point.x,point.y);
          }

          Hide();

          // If this is the 2nd WM_LBUTTONDOWN in x milliseconds,
          // post a WM_LBUTTONDBLCLK message instead of a single click.
          pWnd->PostMessage(  bDoubleClick ? WM_LBUTTONDBLCLK : pMsg->message,
                            pMsg->wParam,
                            pMsg->lParam);
          return TRUE;   
	   }
   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:
        Hide();
      m_pParentWnd->PostMessage( pMsg->message, pMsg->wParam, pMsg->lParam );
      return TRUE;
   }

   if( GetFocus() == NULL )
   {
        Hide();
      return TRUE;
   }

   return CWnd::PreTranslateMessage(pMsg);
}

#endif // GRIDCONTROL_NO_TITLETIPS
