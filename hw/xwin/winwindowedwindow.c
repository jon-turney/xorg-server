/*
  File: winwindowedwindow.c
  Purpose: Screen window functions for windowed mode

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice (including the next
  paragraph) shall be included in all copies or substantial portions of the
  Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
  At the moment, these only need to do do things when the window is an AIGLX
  window
*/

/*
 * MapWindow - See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowWindowed(WindowPtr pWin)
{
  Bool fResult = TRUE;
  ScreenPtr pScreen = pWin->drawable.pScreen;
  winWindowPriv(pWin);
  winScreenPriv(pScreen);

  winDebug("winMapWindowWindowed: pWin 0x%08x XID 0x%08x\n", pWin, pWin->drawable.id);

  WIN_UNWRAP(RealizeWindow);
  fResult =  (*pScreen->RealizeWindow)(pWin);
  WIN_WRAP(RealizeWindow, winMapWindowWindowed);

  /* Check if we need to show the window */
  if (pWinPriv->fWglUsed && pWinPriv->hWnd)
    ShowWindow(pWinPriv->hWnd, SW_SHOW);

  return fResult;
}

/*
 * UnmapWindow - See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowWindowed (WindowPtr pWin)
{
  Bool			fResult = TRUE;
  ScreenPtr		pScreen = pWin->drawable.pScreen;
  winWindowPriv(pWin);
  winScreenPriv(pScreen);

  winDebug("winUnmapWindowWindowed: pWin 0x%08x XID 0x%08x\n", pWin, pWin->drawable.id);

  WIN_UNWRAP(UnrealizeWindow);
  fResult = (*pScreen->UnrealizeWindow)(pWin);
  WIN_WRAP(UnrealizeWindow, winUnmapWindowWindowed);

  /* Check if we need to hide the window */
  if (pWinPriv->fWglUsed && pWinPriv->hWnd)
    ShowWindow(pWinPriv->hWnd, SW_HIDE);

  return fResult;
}

/*
 * PositionWindow - See Porting Layer Definition - p. 37
 */

Bool
winPositionWindowWindowed(WindowPtr pWin, int x, int y)
{
  Bool fResult = TRUE;
  ScreenPtr pScreen = pWin->drawable.pScreen;
  winWindowPriv(pWin);
  winScreenPriv(pScreen);

  WIN_UNWRAP(PositionWindow);
  fResult = (*pScreen->PositionWindow)(pWin, x, y);
  WIN_WRAP(PositionWindow, winPositionWindowWindowed);

  if (pWinPriv->fWglUsed && pWinPriv->hWnd)
    {
      MoveWindow(pWinPriv->hWnd,
                 pWin->drawable.x,
                 pWin->drawable.y,
                 pWin->drawable.width,
                 pWin->drawable.height,
                 FALSE);
    }

  return fResult;
}

/*
 * DestroyWindow - See Porting Layer Definition - p. 37
 */

Bool
winDestroyWindowWindowed(WindowPtr pWin)
{
  Bool fResult = TRUE;
  ScreenPtr pScreen = pWin->drawable.pScreen;
  winWindowPriv(pWin);
  winScreenPriv(pScreen);

  WIN_UNWRAP(DestroyWindow);
  fResult = (*pScreen->DestroyWindow)(pWin);
  WIN_WRAP(DestroyWindow, winDestroyWindowWindowed);

  if (pWinPriv->fWglUsed && pWinPriv->hWnd)
    {
      DestroyWindow(pWinPriv->hWnd);
      pWinPriv->hWnd = NULL;
      pWinPriv->fWglUsed = FALSE;
    }

  return fResult;
}
