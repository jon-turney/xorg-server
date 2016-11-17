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

#include <X11/Xwindows.h>

#include "win.h"
#include "winmsg.h"
#include <cursorstr.h>

#include "cursor_convert.h"

#define BRIGHTNESS(x) (x##Red * 0.299 + x##Green * 0.587 + x##Blue * 0.114)

#if 1
#define WIN_DEBUG_MSG winDebug
#else
#define WIN_DEBUG_MSG(...)
#endif

static unsigned char
reverse(unsigned char c)
{
    int i;
    unsigned char ret = 0;

    for (i = 0; i < 8; ++i) {
        ret |= ((c >> i) & 1) << (7 - i);
    }
    return ret;
}

/*
 * Convert X cursor to Windows cursor
 * FIXME: Perhaps there are more smart code
 */
HCURSOR
winXCursorToHCURSOR(WMUTIL_CURSOR *pCursor)
{
    HCURSOR hCursor = NULL;
    unsigned char *pAnd;
    unsigned char *pXor;
    int nCX, nCY;
    int nBytes;
    double dForeY, dBackY;
    BOOL fReverse;
    HBITMAP hAnd, hXor;
    ICONINFO ii;
    unsigned char *pCur;
    unsigned char bit;
    HDC hDC;
    BITMAPV4HEADER bi;
    BITMAPINFO *pbmi;
    uint32_t *lpBits;

    int sm_cx = GetSystemMetrics(SM_CXCURSOR);
    int sm_cy = GetSystemMetrics(SM_CYCURSOR);

    WIN_DEBUG_MSG("winXCursorToHCURSOR: Win32 size: %dx%d X11 size: %dx%d hotspot: %d,%d\n",
                  sm_cx, sm_cy,
                  pCursor->width, pCursor->height,
                  pCursor->xhot, pCursor->yhot);

    /* We can use only White and Black, so calc brightness of color
     * Also check if the cursor is inverted */
    dForeY = BRIGHTNESS(pCursor->fore);
    dBackY = BRIGHTNESS(pCursor->back);
    fReverse = dForeY < dBackY;

    /* Check whether the X11 cursor is bigger than the win32 cursor */
    if (sm_cx < pCursor->width ||
        sm_cy < pCursor->height) {
        winError("winXCursorToHCURSOR - Windows requires %dx%d cursor but X requires %dx%d\n",
                 sm_cx, sm_cy,
                 pCursor->width, pCursor->height);
    }

    /* Get the number of bytes required to store the whole cursor image
     * This is roughly (sm_cx * sm_cy) / 8
     * round up to 8 pixel boundary so we can convert whole bytes */
    nBytes =
        bits_to_bytes(sm_cx) * sm_cy;

    /* Get the effective width and height */
    nCX = min(sm_cx, pCursor->width);
    nCY = min(sm_cy, pCursor->height);

    /* Allocate memory for the bitmaps */
    pAnd = malloc(nBytes);
    memset(pAnd, 0xFF, nBytes);
    pXor = calloc(1, nBytes);
    memset(pXor, 0x00, nBytes);

    /* prepare the pointers */
    hCursor = NULL;
    lpBits = NULL;

    /* We have a truecolor alpha-blended cursor and can use it! */
    if (pCursor->argb) {
        WIN_DEBUG_MSG("winXCursorToHCURSOR: Trying truecolor alphablended cursor\n");
        memset(&bi, 0, sizeof(BITMAPV4HEADER));
        bi.bV4Size = sizeof(BITMAPV4HEADER);
        bi.bV4Width = sm_cx;
        bi.bV4Height = -(sm_cy);    /* right-side up */
        bi.bV4Planes = 1;
        bi.bV4BitCount = 32;
        bi.bV4V4Compression = BI_BITFIELDS;
        bi.bV4RedMask = 0x00FF0000;
        bi.bV4GreenMask = 0x0000FF00;
        bi.bV4BlueMask = 0x000000FF;
        bi.bV4AlphaMask = 0xFF000000;

        lpBits = calloc(sm_cx * sm_cy, sizeof(uint32_t));

        if (lpBits) {
            int y;
            for (y = 0; y < nCY; y++) {
                void *src, *dst;
                src = &(pCursor->argb[y * pCursor->width]);
                dst = &(lpBits[y * sm_cx]);
                memcpy(dst, src, 4 * nCX);
            }
        }
    }                           /* End if-truecolor-icon */
    else
    {
        /* Convert the X11 bitmap to a win32 bitmap
         * The first is for an empty mask */
        if (pCursor->emptyMask) {
          int x, y, xmax = bits_to_bytes(nCX);

          for (y = 0; y < nCY; ++y)
            for (x = 0; x < xmax; ++x) {
              int nWinPix = bits_to_bytes(sm_cx) * y + x;
              int nXPix = BitmapBytePad(pCursor->width) * y + x;

              pAnd[nWinPix] = 0;
              if (fReverse)
                pXor[nWinPix] = reverse(~pCursor->source[nXPix]);
              else
                pXor[nWinPix] = reverse(pCursor->source[nXPix]);
            }
        }
        else {
          int x, y, xmax = bits_to_bytes(nCX);

          for (y = 0; y < nCY; ++y)
            for (x = 0; x < xmax; ++x) {
              int nWinPix = bits_to_bytes(sm_cx) * y + x;
              int nXPix = BitmapBytePad(pCursor->width) * y + x;

              unsigned char mask = pCursor->mask[nXPix];

              pAnd[nWinPix] = reverse(~mask);
              if (fReverse)
                pXor[nWinPix] =
                  reverse(~pCursor->source[nXPix] & mask);
              else
                pXor[nWinPix] =
                  reverse(pCursor->source[nXPix] & mask);
            }
        }
    }

    if (!lpBits) {
        RGBQUAD *pbmiColors;
        /* Bicolor, use a palettized DIB */
        WIN_DEBUG_MSG("winXCursorToHCURSOR: Trying two color cursor\n");
        pbmi = (BITMAPINFO *) &bi;
        pbmiColors = &(pbmi->bmiColors[0]);

        memset(pbmi, 0, sizeof(BITMAPINFOHEADER));
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth = sm_cx;
        pbmi->bmiHeader.biHeight = -abs(sm_cy);     /* right-side up */
        pbmi->bmiHeader.biPlanes = 1;
        pbmi->bmiHeader.biBitCount = 8;
        pbmi->bmiHeader.biCompression = BI_RGB;
        pbmi->bmiHeader.biSizeImage = 0;
        pbmi->bmiHeader.biClrUsed = 3;
        pbmi->bmiHeader.biClrImportant = 3;

        pbmiColors[0].rgbRed = 0;  /* Empty */
        pbmiColors[0].rgbGreen = 0;
        pbmiColors[0].rgbBlue = 0;
        pbmiColors[0].rgbReserved = 0;
        pbmiColors[1].rgbRed = pCursor->backRed >> 8;      /* Background */
        pbmiColors[1].rgbGreen = pCursor->backGreen >> 8;
        pbmiColors[1].rgbBlue = pCursor->backBlue >> 8;
        pbmiColors[1].rgbReserved = 0;
        pbmiColors[2].rgbRed = pCursor->foreRed >> 8;      /* Foreground */
        pbmiColors[2].rgbGreen = pCursor->foreGreen >> 8;
        pbmiColors[2].rgbBlue = pCursor->foreBlue >> 8;
        pbmiColors[2].rgbReserved = 0;

        lpBits = calloc(sm_cx * sm_cy, 1);

        pCur = (unsigned char *) lpBits;
        if (lpBits) {
	    int x, y;
            for (y = 0; y < sm_cy; y++) {
                for (x = 0; x < sm_cx; x++) {
                    if (x >= nCX || y >= nCY)   /* Outside of X11 icon bounds */
                        (*pCur++) = 0;
                    else {      /* Within X11 icon bounds */

                        int nWinPix =
                            bits_to_bytes(sm_cx) * y +
                            (x / 8);

                        bit = pAnd[nWinPix];
                        bit = bit & (1 << (7 - (x & 7)));
                        if (!bit) {     /* Within the cursor mask? */
                            int nXPix =
                                BitmapBytePad(pCursor->width) * y +
                                (x / 8);
                            bit =
                                ~reverse(~pCursor->
                                         source[nXPix] & pCursor->
                                         mask[nXPix]);
                            bit = bit & (1 << (7 - (x & 7)));
                            if (bit)    /* Draw foreground */
                                (*pCur++) = 2;
                            else        /* Draw background */
                                (*pCur++) = 1;
                        }
                        else    /* Outside the cursor mask */
                            (*pCur++) = 0;
                    }
                }               /* end for (x) */
            }                   /* end for (y) */
        }                       /* end if (lpbits) */
    }

    /* If one of the previous two methods gave us the bitmap we need, make a cursor */
    if (lpBits) {
        WIN_DEBUG_MSG("winXCursorToHCURSOR: Creating bitmap cursor\n");

        hAnd = NULL;
        hXor = NULL;

        hAnd =
            CreateBitmap(sm_cx, sm_cy,
                         1, 1, pAnd);

        hDC = GetDC(NULL);
        if (hDC) {
            hXor =
                CreateCompatibleBitmap(hDC, sm_cx,
                                       sm_cy);
            SetDIBits(hDC, hXor, 0, sm_cy, lpBits,
                      (BITMAPINFO *) &bi, DIB_RGB_COLORS);
            ReleaseDC(NULL, hDC);
        }
        free(lpBits);

        if (hAnd && hXor) {
            ii.fIcon = FALSE;
            ii.xHotspot = pCursor->xhot;
            ii.yHotspot = pCursor->yhot;
            ii.hbmMask = hAnd;
            ii.hbmColor = hXor;
            hCursor = (HCURSOR) CreateIconIndirect(&ii);

            if (hCursor == NULL)
                winError("winXCursorToHCURSOR - CreateIconIndirect failed: %x", (unsigned int)GetLastError());
            else {
                /*
                   Apparently, CreateIconIndirect() sometimes creates an Icon instead of a Cursor.
                   This breaks the hotspot and makes the cursor unusable. Discard the broken cursor
                   and revert to simple b&w cursor. (Seen on W2K in 2004...)
                */
                if (GetIconInfo(hCursor, &ii)) {
                    if (ii.fIcon) {
                        winError
                            ("winXCursorToHCURSOR: CreateIconIndirect made an icon, not a cursor. Trying again.\n");

                        DestroyCursor(hCursor);

                        ii.fIcon = FALSE;
                        ii.xHotspot = pCursor->xhot;
                        ii.yHotspot = pCursor->yhot;
                        hCursor = (HCURSOR) CreateIconIndirect(&ii);

                        if (hCursor == NULL)
                            winError("winXCursorToHCURSOR - CreateIconIndirect failed: %x", (unsigned int)GetLastError());
                    }
                    /* GetIconInfo creates new bitmaps. Destroy them again */
                    if (ii.hbmMask)
                        DeleteObject(ii.hbmMask);
                    if (ii.hbmColor)
                        DeleteObject(ii.hbmColor);
                }
            }
        }

        if (hAnd)
            DeleteObject(hAnd);
        if (hXor)
            DeleteObject(hXor);
    }

    if (!hCursor) {
        WIN_DEBUG_MSG("winXCursorToHCURSOR: Creating b&w cursor\n");
        /* We couldn't make a color cursor for this screen, use
           black and white instead */
        hCursor = CreateCursor(GetModuleHandle(NULL),
                               pCursor->xhot, pCursor->yhot,
                               sm_cx,
                               sm_cy, pAnd, pXor);
        if (hCursor == NULL)
            winError("winXCursorToHCURSOR - CreateCursor failed: %x", (unsigned int)GetLastError());
    }
    free(pAnd);
    free(pXor);

    return hCursor;
}
