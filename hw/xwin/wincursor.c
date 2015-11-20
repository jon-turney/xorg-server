/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "winmsg.h"
#include <cursorstr.h>
#include <mipointrst.h>
#include <servermd.h>
#include "misc.h"
#include "wmutil/cursor_convert.h"

#if 0
#define WIN_DEBUG_MSG winDebug
#else
#define WIN_DEBUG_MSG(...)
#endif

/*
 * Local function prototypes
 */

static void
 winPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);

static Bool
 winCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y);

static void
 winCrossScreen(ScreenPtr pScreen, Bool fEntering);

miPointerScreenFuncRec g_winPointerCursorFuncs = {
    winCursorOffScreen,
    winCrossScreen,
    winPointerWarpCursor
};

static void
winPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    winScreenPriv(pScreen);
    RECT rcClient;
    static Bool s_fInitialWarp = TRUE;

    /* Discard first warp call */
    if (s_fInitialWarp) {
        /* First warp moves mouse to center of window, just ignore it */

        /* Don't ignore subsequent warps */
        s_fInitialWarp = FALSE;

        winErrorFVerb(2,
                      "winPointerWarpCursor - Discarding first warp: %d %d\n",
                      x, y);

        return;
    }

    /*
       Only update the Windows cursor position if root window is active,
       or we are in a rootless mode
     */
    if ((pScreenPriv->hwndScreen == GetForegroundWindow())
        || pScreenPriv->pScreenInfo->fRootless
#ifdef XWIN_MULTIWINDOW
        || pScreenPriv->pScreenInfo->fMultiWindow
#endif
        ) {
        /* Get the client area coordinates */
        GetClientRect(pScreenPriv->hwndScreen, &rcClient);

        /* Translate the client area coords to screen coords */
        MapWindowPoints(pScreenPriv->hwndScreen,
                        HWND_DESKTOP, (LPPOINT) &rcClient, 2);

        /*
         * Update the Windows cursor position so that we don't
         * immediately warp back to the current position.
         */
        SetCursorPos(rcClient.left + x, rcClient.top + y);
    }

    /* Call the mi warp procedure to do the actual warping in X. */
    miPointerWarpCursor(pDev, pScreen, x, y);
}

static Bool
winCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
winCrossScreen(ScreenPtr pScreen, Bool fEntering)
{
}

/*
 * Convert X cursor to Windows cursor
 */
static HCURSOR
winLoadCursor(ScreenPtr pScreen, CursorPtr pCursor, int screen)
{
    WMUTIL_CURSOR cursor;

    cursor.width = pCursor->bits->width;
    cursor.height = pCursor->bits->height;
    cursor.xhot = pCursor->bits->xhot;
    cursor.yhot = pCursor->bits->yhot;
    cursor.argb = (uint32_t *)pCursor->bits->argb;
    cursor.source = pCursor->bits->source;
    cursor.mask = pCursor->bits->mask;
    cursor.emptyMask = pCursor->bits->emptyMask;
    cursor.foreRed = pCursor->foreRed;
    cursor.foreGreen = pCursor->foreGreen;
    cursor.foreBlue = pCursor->foreBlue;
    cursor.backRed = pCursor->backRed;
    cursor.backGreen = pCursor->backGreen;
    cursor.backBlue = pCursor->backBlue;

    return winXCursorToHCURSOR(&cursor);
}

/*
===========================================================================

 Pointer sprite functions

===========================================================================
*/

/*
 * winRealizeCursor
 *  Convert the X cursor representation to native format if possible.
 */
static Bool
winRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    if (pCursor == NULL || pCursor->bits == NULL)
        return FALSE;

    /* FIXME: cache ARGB8888 representation? */

    return TRUE;
}

/*
 * winUnrealizeCursor
 *  Free the storage space associated with a realized cursor.
 */
static Bool
winUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

/*
 * winSetCursor
 *  Set the cursor sprite and position.
 */
static void
winSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, int x,
             int y)
{
    POINT ptCurPos, ptTemp;
    HWND hwnd;
    RECT rcClient;
    BOOL bInhibit;

    winScreenPriv(pScreen);
    WIN_DEBUG_MSG("winSetCursor: cursor=%p\n", pCursor);

    /* Inhibit changing the cursor if the mouse is not in a client area */
    bInhibit = FALSE;
    if (GetCursorPos(&ptCurPos)) {
        hwnd = WindowFromPoint(ptCurPos);
        if (hwnd) {
            if (GetClientRect(hwnd, &rcClient)) {
                ptTemp.x = rcClient.left;
                ptTemp.y = rcClient.top;
                if (ClientToScreen(hwnd, &ptTemp)) {
                    rcClient.left = ptTemp.x;
                    rcClient.top = ptTemp.y;
                    ptTemp.x = rcClient.right;
                    ptTemp.y = rcClient.bottom;
                    if (ClientToScreen(hwnd, &ptTemp)) {
                        rcClient.right = ptTemp.x;
                        rcClient.bottom = ptTemp.y;
                        if (!PtInRect(&rcClient, ptCurPos))
                            bInhibit = TRUE;
                    }
                }
            }
        }
    }

    if (pCursor == NULL) {
        if (pScreenPriv->cursor.visible) {
            if (!bInhibit && g_fSoftwareCursor)
                ShowCursor(FALSE);
            pScreenPriv->cursor.visible = FALSE;
        }
    }
    else {
        if (pScreenPriv->cursor.handle) {
            if (!bInhibit)
                SetCursor(NULL);
            DestroyCursor(pScreenPriv->cursor.handle);
            pScreenPriv->cursor.handle = NULL;
        }
        pScreenPriv->cursor.handle =
            winLoadCursor(pScreen, pCursor, pScreen->myNum);
        WIN_DEBUG_MSG("winSetCursor: handle=%p\n", pScreenPriv->cursor.handle);

        if (!bInhibit)
            SetCursor(pScreenPriv->cursor.handle);

        if (!pScreenPriv->cursor.visible) {
            if (!bInhibit && g_fSoftwareCursor)
                ShowCursor(TRUE);
            pScreenPriv->cursor.visible = TRUE;
        }
    }
}

/*
 * winMoveCursor
 *  Move the cursor. This is a noop for us.
 */
static void
winMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
}

static Bool
winDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScr)
{
    winScreenPriv(pScr);
    return pScreenPriv->cursor.spriteFuncs->DeviceCursorInitialize(pDev, pScr);
}

static void
winDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScr)
{
    winScreenPriv(pScr);
    pScreenPriv->cursor.spriteFuncs->DeviceCursorCleanup(pDev, pScr);
}

static miPointerSpriteFuncRec winSpriteFuncsRec = {
    winRealizeCursor,
    winUnrealizeCursor,
    winSetCursor,
    winMoveCursor,
    winDeviceCursorInitialize,
    winDeviceCursorCleanup
};

/*
===========================================================================

 Other screen functions

===========================================================================
*/

/*
 * winCursorQueryBestSize
 *  Handle queries for best cursor size
 */
static void
winCursorQueryBestSize(int class, unsigned short *width,
                       unsigned short *height, ScreenPtr pScreen)
{
    winScreenPriv(pScreen);

    if (class == CursorShape) {
        *width = pScreenPriv->cursor.sm_cx;
        *height = pScreenPriv->cursor.sm_cy;
    }
    else {
        if (pScreenPriv->cursor.QueryBestSize)
            (*pScreenPriv->cursor.QueryBestSize) (class, width, height,
                                                  pScreen);
    }
}

/*
 * winInitCursor
 *  Initialize cursor support
 */
Bool
winInitCursor(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    miPointerScreenPtr pPointPriv;

    /* override some screen procedures */
    pScreenPriv->cursor.QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = winCursorQueryBestSize;

    pPointPriv = (miPointerScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

    pScreenPriv->cursor.spriteFuncs = pPointPriv->spriteFuncs;
    pPointPriv->spriteFuncs = &winSpriteFuncsRec;

    pScreenPriv->cursor.handle = NULL;
    pScreenPriv->cursor.visible = FALSE;

    pScreenPriv->cursor.sm_cx = GetSystemMetrics(SM_CXCURSOR);
    pScreenPriv->cursor.sm_cy = GetSystemMetrics(SM_CYCURSOR);

    return TRUE;
}
