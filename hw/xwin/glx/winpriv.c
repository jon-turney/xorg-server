/*
 * Export window information for the Windows-OpenGL GLX implementation.
 *
 * Authors: Alexander Gottwald
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winpriv.h"
#include "winwindow.h"

void
winCreateWindowsWindow (WindowPtr pWin);

static
void
winCreateWindowsWindowHierarchy(WindowPtr pWin)
{
  winWindowPriv(pWin);

  winDebug("winCreateWindowsWindowHierarchy - pWin:%08x XID:0x%x \n", pWin, pWin->drawable.id);

  /* recursively ensure parent window exists if it's not the root window */
  if (pWin->parent)
    {
      if (pWin->parent != pWin->drawable.pScreen->root)
        winCreateWindowsWindowHierarchy(pWin->parent);
    }

  /* ensure this window exists */
  if (pWinPriv->hWnd == NULL)
    {
      winCreateWindowsWindow(pWin);

      /* ... and if it's already been mapped, make sure it's visible */
      if (pWin->mapped)
        {
          /* Display the window without activating it */
          if (pWin->drawable.class != InputOnly)
            ShowWindow (pWinPriv->hWnd, SW_SHOWNOACTIVATE);

          /* Send first paint message */
          UpdateWindow (pWinPriv->hWnd);
        }
    }
}

static
LRESULT CALLBACK
winGlChildWindowProc (HWND hwnd, UINT message,
                      WPARAM wParam, LPARAM lParam)
{
  WindowPtr pWin;
  ScreenPtr pScreen = NULL;

#if CYGDEBUG
  winDebugWin32Message("winGlChildWindowProc", hwnd, message, wParam, lParam);
#endif

  /* If our X window pointer is valid */
  if ((pWin = GetProp(hwnd, WIN_WINDOW_PROP)) != NULL)
    {
      /* Get pointers to the drawable and the screen */
      pScreen = pWin->drawable.pScreen;
    }

  switch (message)
    {
    case WM_CREATE:
      {
        pWin = ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetProp(hwnd, WIN_WINDOW_PROP, (HANDLE)(pWin));
        SetProp(hwnd, WIN_WID_PROP, (HANDLE)winGetWindowID(pWin));
      }
      break;

    case WM_ERASEBKGND:
      return TRUE;


  return DefWindowProc (hwnd, message, wParam, lParam);
}

static
void
winCreateWindowedWindow(WindowPtr pWin, winPrivScreenPtr pWinScreen)
{
  winWindowPriv(pWin);

  // create window class if needed
#define WIN_GL_WINDOW_CLASS "cygwin/x X child GL window"
  {
    static wATOM glChildWndClass = 0;
    if (glChildWndClass == 0)
      {
        WNDCLASSEX wc;
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = winGlChildWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hIcon = 0;
        wc.hCursor = 0;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WIN_GL_WINDOW_CLASS;
        wc.hIconSm = 0;
        RegisterClassEx (&wc);
      }
  }

  // ensure this window exists */
  if (pWinPriv->hWnd == NULL)
    {
      int ExtraClass = (pWin->realized) ? WS_VISIBLE : 0;
      pWinPriv->hWnd = CreateWindowExA(WS_EX_TRANSPARENT,
                                       WIN_GL_WINDOW_CLASS,
                                       "",
                                       WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN  | WS_DISABLED | ExtraClass,
                                       pWin->drawable.x,
                                       pWin->drawable.y,
                                       pWin->drawable.width,
                                       pWin->drawable.height,
                                       pWinScreen->hwndScreen,
                                       (HMENU) NULL,
                                       g_hInstance,
                                       pWin);
    }
}

/**
 * Return size and handles of a window.
 * If pWin is NULL, then the information for the root window is requested.
 */
HWND winGetWindowInfo(WindowPtr pWin)
{
    winTrace("%s: pWin %p XID 0x%x\n", __FUNCTION__, pWin, pWin->drawable.id);

    /* a real window was requested */
    if (pWin != NULL)
    {
        /* Get the window and screen privates */
        ScreenPtr pScreen = pWin->drawable.pScreen;
        winPrivScreenPtr pWinScreen = winGetScreenPriv(pScreen);
        winScreenInfoPtr pScreenInfo = NULL;

        if (pWinScreen == NULL)
        {
            ErrorF("winGetWindowInfo: screen has no privates\n");
            return NULL;
        }

        winWindowPriv(pWin);
        if (pWinPriv == NULL)
          {
            ErrorF("winGetWindowInfo: window has no privates\n");
            return pWinScreen->hwndScreen;
          }

        pScreenInfo = pWinScreen->pScreenInfo;
#ifdef XWIN_MULTIWINDOW
        /* check for multiwindow mode */
        if (pScreenInfo->fMultiWindow)
        {
            if (pWinPriv->hWnd == NULL)
            {
              ErrorF("winGetWindowInfo: forcing window to exist\n");
              winCreateWindowsWindowHierarchy(pWin);
            }

            if (pWinPriv->hWnd != NULL)
              {
                /* mark GLX active on that hwnd */
                pWinPriv->fWglUsed = TRUE;
              }

            return pWinPriv->hWnd;
        }
        else
#endif
#ifdef XWIN_MULTIWINDOWEXTWM
        /* check for multiwindow external wm mode */
        if (pScreenInfo->fMWExtWM)
        {
            win32RootlessWindowPtr pRLWinPriv
                = (win32RootlessWindowPtr) RootlessFrameForWindow (pWin, FALSE);

            if (pRLWinPriv == NULL)
            {
                ErrorF("winGetWindowInfo: window has no privates\n");
                return pWinScreen->hwndScreen;
            }

            return pRLWinPriv->hWnd;
        }
        else
#endif
        /* check for windowed mode */
        if (TRUE
#ifdef XWIN_MULTIWINDOW
	      && !pScreenInfo->fMultiWindow
#endif
#ifdef XWIN_MULTIWINDOWEXTWM
	      && !pScreenInfo->fMWExtWM
#endif
	      && !pScreenInfo->fRootless)
        {
          if (pWinPriv->hWnd == NULL)
            {
              winCreateWindowedWindow(pWin, pWinScreen);
            }

          if (pWinPriv->hWnd != NULL)
            {
              /* mark GLX active on that hwnd */
              pWinPriv->fWglUsed = TRUE;
            }

          return pWinPriv->hWnd;
        }
    }
    else
    {
        ScreenPtr pScreen = g_ScreenInfo[0].pScreen;
        winPrivScreenPtr pWinScreen = winGetScreenPriv(pScreen);

        if (pWinScreen == NULL)
        {
            ErrorF("winGetWindowInfo: screen has no privates\n");
            return NULL;
        }

        ErrorF("winGetWindowInfo: returning root window\n");

        return pWinScreen->hwndScreen;
    }

    return NULL;
}

Bool
winCheckScreenAiglxIsSupported(ScreenPtr pScreen)
{
  winPrivScreenPtr pWinScreen = winGetScreenPriv(pScreen);
  winScreenInfoPtr pScreenInfo = pWinScreen->pScreenInfo;

#ifdef XWIN_MULTIWINDOW
  if (pScreenInfo->fMultiWindow)
    return TRUE;
#endif

#ifdef XWIN_MULTIWINDOWEXTWM
  if (pScreenInfo->fMWExtWM)
    return TRUE;
#endif

  if (TRUE
#ifdef XWIN_MULTIWINDOW
      && !pScreenInfo->fMultiWindow
#endif
#ifdef XWIN_MULTIWINDOWEXTWM
      && !pScreenInfo->fMWExtWM
#endif
      && !pScreenInfo->fRootless)
    return TRUE;

  return FALSE;
  /* I think that adds up to return !pScreenInfo->fRootless :-)  */
}
