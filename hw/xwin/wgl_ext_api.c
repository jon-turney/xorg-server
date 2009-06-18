/*
 * File: wgl_ext_api.c
 * Purpose: Wrapper functions for Win32 OpenGL wgl extension functions
 *
 * Authors: Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 *
 *
 * Permission is hereby granted, free of chabrge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glx/glxserver.h>
#include <glx/glxext.h>
#include <wgl_ext_api.h>

#define RESOLVE_RET(procname, symbol, retval) \
    static wBOOL init = TRUE; \
    static __stdcall procname proc = NULL; \
    if (init) { \
        proc = (procname)wglGetProcAddress(symbol); \
        init = FALSE; \
        if (proc == NULL) { \
            ErrorF("glwrap: Can't resolve \"%s\"\n", symbol); \
        } else \
            ErrorF("glwrap: resolved \"%s\"\n", symbol); \
    } \
    if (proc == NULL) { \
        __glXErrorCallBack(0); \
        return retval; \
    }

#define RESOLVE(procname, symbol) RESOLVE_RET(procname, symbol,)

/*
 * There are extensions to the wgl*() API as well; again we call
 * these functions by using wglGetProcAddress() to get a pointer
 * to the function, and wrapping it for cdecl/stdcall conversion
 */

typedef char *(__stdcall *PFNWGLGETEXTENSIONSSTRINGARB)(HDC hdc);

const char *wglGetExtensionsStringARBWrapper(HDC hdc)
{
  RESOLVE_RET(PFNWGLGETEXTENSIONSSTRINGARB, "wglGetExtensionsStringARB", "");
  return proc(hdc);
}

typedef wBOOL (__stdcall *PFNWGLMAKECONTEXTCURRENTARB)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);

wBOOL wglMakeContextCurrentARBWrapper(HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
{
  RESOLVE_RET(PFNWGLMAKECONTEXTCURRENTARB, "wglMakeContextCurrentARB", FALSE);
  return proc(hDrawDC, hReadDC, hglrc);
}

