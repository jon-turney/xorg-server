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
 * Authors:	Earle F. Philhower, III
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <X11/Xwindows.h>
#include <X11/Xlib.h>

#include "winresource.h"
#include "winprefs.h"
#include "winmultiwindowicons.h"
#include "winglobals.h"
#include "wmutil/icon_convert.h"

/*
 * global variables
 */
extern HINSTANCE g_hInstance;

/*
 * Change the Windows window icon
 */

#ifdef XWIN_MULTIWINDOW
void
winUpdateIcon(HWND hWnd, xcb_connection_t *conn, Window id, HICON hIconNew)
{
    HICON hIcon, hIconSmall = NULL, hIconOld;

    if (hIconNew)
      {
        /* Start with the icon from preferences, if any */
        hIcon = hIconNew;
        hIconSmall = hIconNew;
      }
    else
      {
        /* If we still need an icon, try and get the icon from WM_HINTS */
        hIcon = winXIconToHICON(conn, id, GetSystemMetrics(SM_CXICON));
        hIconSmall = winXIconToHICON(conn, id, GetSystemMetrics(SM_CXSMICON));

        /* If we got the small, but not the large one swap them */
        if (!hIcon && hIconSmall) {
            hIcon = hIconSmall;
            hIconSmall = NULL;
        }
      }

    /* If we still need an icon, use the default one */
    if (!hIcon) {
        hIcon = g_hIconX;
        hIconSmall = g_hSmallIconX;
    }

    if (hIcon) {
        /* Set the large icon */
        hIconOld = (HICON) SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
        /* Delete the old icon if its not the default */
        winDestroyIcon(hIconOld);
    }

    if (hIconSmall) {
        /* Same for the small icon */
        hIconOld =
            (HICON) SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIconSmall);
        winDestroyIcon(hIconOld);
    }
}

void
winInitGlobalIcons(void)
{
    int sm_cx = GetSystemMetrics(SM_CXICON);
    int sm_cxsm = GetSystemMetrics(SM_CXSMICON);

    /* Load default X icon in case it's not ready yet */
    if (!g_hIconX) {
        g_hIconX = winOverrideDefaultIcon(sm_cx);
        g_hSmallIconX = winOverrideDefaultIcon(sm_cxsm);
    }

    if (!g_hIconX) {
        g_hIconX = (HICON) LoadImage(g_hInstance,
                                     MAKEINTRESOURCE(IDI_XWIN),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON), 0);
        g_hSmallIconX = (HICON) LoadImage(g_hInstance,
                                          MAKEINTRESOURCE(IDI_XWIN),
                                          IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          LR_DEFAULTSIZE);
    }
}

void
winSelectIcons(HICON * pIcon, HICON * pSmallIcon)
{
    HICON hIcon, hSmallIcon;

    winInitGlobalIcons();

    /* Use default X icon */
    hIcon = g_hIconX;
    hSmallIcon = g_hSmallIconX;

    if (pIcon)
        *pIcon = hIcon;

    if (pSmallIcon)
        *pSmallIcon = hSmallIcon;
}

void
winDestroyIcon(HICON hIcon)
{
    /* Delete the icon if its not one of the application defaults or an override */
    if (hIcon &&
        hIcon != g_hIconX &&
        hIcon != g_hSmallIconX && !winIconIsOverride(hIcon))
        DestroyIcon(hIcon);
}
#endif
