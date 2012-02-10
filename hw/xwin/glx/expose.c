/*
 * Copyright (C) Jon TURNEY 2012
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * Send an expose event for a given RECT on a HWND
 *
 * We use this to make a window with some static OpenGL content redraw it
 * when the image on the screen is damaged (since we don't have those bits
 * in the framebuffer to refresh ourselves)
 *
 */
void
winExposeWindow(ScreenPtr pScreen, WindowPtr pWin, HWND hwnd, RECT *rcPaint)
{
  POINT pt;
  BoxRec b;
  RegionPtr r;
  winPrivScreenPtr pWinScreen = winGetScreenPriv(pScreen);

  /* Translate the window coordinates to screen window coordinates */
  pt.x = rcPaint->left;
  pt.y = rcPaint->top;
  winDebug("winExposeWindow: client pt %d %d\n", pt.x, pt.y);
  MapWindowPoints(hwnd, pWinScreen->hwndScreen, &pt, 1);
  winDebug("winExposeWindow: screen pt %d %d\n", pt.x, pt.y);

  /* Construct a box for invalidated area */
  b.x1 = pt.x;
  b.y1 = pt.y;
  b.x2 = b.x1 + (rcPaint->right - rcPaint->left);
  b.y2 = b.y1 + (rcPaint->bottom - rcPaint->top);
  winDebug("winExposeWindow: w %d h %d\n", b.x2-b.x1, b.y2-b.y1);

  /* Send an expose event */
  r = RegionCreate(&b, 1);
  (pScreen->WindowExposures)(pWin, NullRegion, r);
  RegionDestroy(r);
}
