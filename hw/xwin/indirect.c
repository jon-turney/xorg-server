/*
 * File: indirect.c
 * Purpose: A GLX implementation that uses Windows OpenGL library
 *
 * Authors: Alexander Gottwald
 *          Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 * Copyright (c) Alexander Gottwald 2004
 *
 * Portions of this file are copied from GL/apple/indirect.c,
 * which contains the following copyright:
 *
 * Copyright (c) 2007, 2008, 2009 Apple Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
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

#include "glwindows.h"
#include <stdint.h>

#include <winpriv.h>
#include <wgl_ext_api.h>

/* ---------------------------------------------------------------------- */
/*
 * Various debug helpers
 */

#define GLWIN_DEBUG_HWND(hwnd)  \
    if (glWinDebugSettings.dumpHWND) { \
        char buffer[1024]; \
        if (GetWindowText(hwnd, buffer, sizeof(buffer))==0) *buffer=0; \
        GLWIN_DEBUG_MSG("Got HWND %p for window '%s'", hwnd, buffer); \
    }

glWinDebugSettingsRec glWinDebugSettings = { 1, 1, 0, 1, 0, 0};

static void glWinInitDebugSettings(void)
{
    char *envptr;

    envptr = getenv("GLWIN_ENABLE_DEBUG");
    if (envptr != NULL)
        glWinDebugSettings.enableDebug = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_ENABLE_TRACE");
    if (envptr != NULL)
        glWinDebugSettings.enableTrace = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_PFD");
    if (envptr != NULL)
        glWinDebugSettings.dumpPFD = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_HWND");
    if (envptr != NULL)
        glWinDebugSettings.dumpHWND = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_DC");
    if (envptr != NULL)
        glWinDebugSettings.dumpDC = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_ENABLE_GLCALL_TRACE");
    if (envptr != NULL)
        glWinDebugSettings.enableGLcallTrace = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DEBUG_ALL");
    if (envptr != NULL)
      {
        glWinDebugSettings.enableDebug = 1;
        glWinDebugSettings.enableTrace = 1;
        glWinDebugSettings.dumpPFD = 1;
        glWinDebugSettings.dumpHWND = 1;
        glWinDebugSettings.dumpDC = 1;
        glWinDebugSettings.enableGLcallTrace = 1;
      }
}

static char errorbuffer[1024];

static
const char *glWinErrorMessage(void)
{
    if (!FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &errorbuffer,
                sizeof(errorbuffer),
                NULL ))
    {
        snprintf(errorbuffer, sizeof(errorbuffer), "Unknown error in FormatMessage: %08x!", (unsigned)GetLastError());
    }
    return errorbuffer;
}

static void pfdOut(const PIXELFORMATDESCRIPTOR *pfd);

#define DUMP_PFD_FLAG(flag) \
    if (pfd->dwFlags & flag) { \
        ErrorF("%s%s", pipesym, #flag); \
        pipesym = " | "; \
    }

static void pfdOut(const PIXELFORMATDESCRIPTOR *pfd)
{
    const char *pipesym = ""; /* will be set after first flag dump */
    ErrorF("PIXELFORMATDESCRIPTOR:\n");
    ErrorF("nSize = %u\n", pfd->nSize);
    ErrorF("nVersion = %u\n", pfd->nVersion);
    ErrorF("dwFlags = %lu = {", pfd->dwFlags);
        DUMP_PFD_FLAG(PFD_MAIN_PLANE);
        DUMP_PFD_FLAG(PFD_OVERLAY_PLANE);
        DUMP_PFD_FLAG(PFD_UNDERLAY_PLANE);
        DUMP_PFD_FLAG(PFD_DOUBLEBUFFER);
        DUMP_PFD_FLAG(PFD_STEREO);
        DUMP_PFD_FLAG(PFD_DRAW_TO_WINDOW);
        DUMP_PFD_FLAG(PFD_DRAW_TO_BITMAP);
        DUMP_PFD_FLAG(PFD_SUPPORT_GDI);
        DUMP_PFD_FLAG(PFD_SUPPORT_OPENGL);
        DUMP_PFD_FLAG(PFD_GENERIC_FORMAT);
        DUMP_PFD_FLAG(PFD_NEED_PALETTE);
        DUMP_PFD_FLAG(PFD_NEED_SYSTEM_PALETTE);
        DUMP_PFD_FLAG(PFD_SWAP_EXCHANGE);
        DUMP_PFD_FLAG(PFD_SWAP_COPY);
        DUMP_PFD_FLAG(PFD_SWAP_LAYER_BUFFERS);
        DUMP_PFD_FLAG(PFD_GENERIC_ACCELERATED);
        DUMP_PFD_FLAG(PFD_DEPTH_DONTCARE);
        DUMP_PFD_FLAG(PFD_DOUBLEBUFFER_DONTCARE);
        DUMP_PFD_FLAG(PFD_STEREO_DONTCARE);
    ErrorF("}\n");

    ErrorF("iPixelType = %hu = %s\n", pfd->iPixelType,
            (pfd->iPixelType == PFD_TYPE_RGBA ? "PFD_TYPE_RGBA" : "PFD_TYPE_COLORINDEX"));
    ErrorF("cColorBits = %hhu\n", pfd->cColorBits);
    ErrorF("cRedBits = %hhu\n", pfd->cRedBits);
    ErrorF("cRedShift = %hhu\n", pfd->cRedShift);
    ErrorF("cGreenBits = %hhu\n", pfd->cGreenBits);
    ErrorF("cGreenShift = %hhu\n", pfd->cGreenShift);
    ErrorF("cBlueBits = %hhu\n", pfd->cBlueBits);
    ErrorF("cBlueShift = %hhu\n", pfd->cBlueShift);
    ErrorF("cAlphaBits = %hhu\n", pfd->cAlphaBits);
    ErrorF("cAlphaShift = %hhu\n", pfd->cAlphaShift);
    ErrorF("cAccumBits = %hhu\n", pfd->cAccumBits);
    ErrorF("cAccumRedBits = %hhu\n", pfd->cAccumRedBits);
    ErrorF("cAccumGreenBits = %hhu\n", pfd->cAccumGreenBits);
    ErrorF("cAccumBlueBits = %hhu\n", pfd->cAccumBlueBits);
    ErrorF("cAccumAlphaBits = %hhu\n", pfd->cAccumAlphaBits);
    ErrorF("cDepthBits = %hhu\n", pfd->cDepthBits);
    ErrorF("cStencilBits = %hhu\n", pfd->cStencilBits);
    ErrorF("cAuxBuffers = %hhu\n", pfd->cAuxBuffers);
    ErrorF("iLayerType = %hhu\n", pfd->iLayerType);
    ErrorF("bReserved = %hhu\n", pfd->bReserved);
    ErrorF("dwLayerMask = %lu\n", pfd->dwLayerMask);
    ErrorF("dwVisibleMask = %lu\n", pfd->dwVisibleMask);
    ErrorF("dwDamageMask = %lu\n", pfd->dwDamageMask);
    ErrorF("\n");
}

/* ---------------------------------------------------------------------- */
/*
 *   structure definitions
 */

typedef struct __GLXWinContext  __GLXWinContext;
typedef struct __GLXWinDrawable __GLXWinDrawable;

struct __GLXWinContext {
  __GLXcontext base;
  HGLRC ctx;                         /* Windows GL Context */
  // HDC dc;                            /* Windows Device Context */
  __GLXWinContext *shareContext;     /* Context with which we will share display lists and textures */
  // winWindowInfoRec winInfo;          /* Window info from XWin DDX - all we actually use at the moment is the HWND, and that changes if the window is unmapped/mapped.... */
  HWND hwnd;                         /* For detecting when HWND has changed (debug output)... */
  PIXELFORMATDESCRIPTOR pfd;         /* Pixel format descriptor */
  //  int pixelFormat;                   /* Pixel format index */
  unsigned isAttached :1;            /* Flag to track if context is attached */
};

struct __GLXWinDrawable
{
  __GLXdrawable base;
  __GLXWinContext *context;
};

typedef struct {
    __GLXscreen base;

    int num_vis;
    __GLcontextModes *modes;
    void **priv;

    /* wrapped screen functions */
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    CopyWindowProcPtr CopyWindow;
} glWinScreen;

/* ---------------------------------------------------------------------- */
/*
 * Forward declarations
 */

static __GLXscreen *glWinScreenProbe(ScreenPtr pScreen);
static __GLXcontext *glWinCreateContext(__GLXscreen *screen,
                                        __GLXconfig *modes,
                                        __GLXcontext *baseShareContext);
static __GLXdrawable *glWinCreateDrawable(__GLXscreen *screen,
                                          DrawablePtr pDraw,
                                          int type,
                                          XID drawId,
                                          __GLXconfig *conf);

static Bool glWinRealizeWindow(WindowPtr pWin);
static Bool glWinUnrealizeWindow(WindowPtr pWin);
static void glWinCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

static HDC glWinMakeDC(__GLXWinContext *gc, __GLXdrawable *draw, HDC *hdc, HWND *hwnd);

static __GLXconfig *glWinCreateConfigs(int *numConfigsPtr, int screenNumber);
static int fbConfigToPixelFormat(__GLXconfig *mode, PIXELFORMATDESCRIPTOR *pfdret);


/* ---------------------------------------------------------------------- */
/*
 * The GLX provider
 */

__GLXprovider __glXWGLProvider = {
    glWinScreenProbe,
    "Win32 native WGL",
    NULL
};

void
glWinPushNativeProvider(void)
{
  GlxPushProvider(&__glXWGLProvider);
}

/* ---------------------------------------------------------------------- */
/*
 * Screen functions
 */

static void
glWinScreenDestroy(__GLXscreen *screen)
{
    GLWIN_DEBUG_MSG("glWinScreenDestroy(%p)", screen);
    __glXScreenDestroy(screen);
    xfree(screen);
}

/* This is called by GlxExtensionInit() asksing the GLX provider if it can handle the screen... */
static __GLXscreen *
glWinScreenProbe(ScreenPtr pScreen)
{
    glWinScreen *screen;

    GLWIN_DEBUG_MSG("glWinScreenProbe");

    glWinInitDebugSettings();

    if (pScreen == NULL)
	return NULL;

    screen = xcalloc(1, sizeof(glWinScreen));

    if (NULL == screen)
	return NULL;

    screen->base.destroy = glWinScreenDestroy;
    screen->base.createContext = glWinCreateContext;
    screen->base.createDrawable = glWinCreateDrawable;
    screen->base.swapInterval = NULL;
    screen->base.hyperpipeFuncs = NULL;
    screen->base.swapBarrierFuncs = NULL;
    screen->base.pScreen = pScreen;
    screen->base.fbconfigs = glWinCreateConfigs(&screen->base.numFBConfigs,
                                                pScreen->myNum);
    // Note that screen->base.numFBConfigs is also initialized by that

    /* These will be set by __glXScreenInit */
    screen->base.visuals = NULL;
    screen->base.numVisuals = 0;

    __glXScreenInit(&screen->base, pScreen);

    /* Wrap RealizeWindow, UnrealizeWindow and CopyWindow on this screen */
    screen->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = glWinRealizeWindow;
    screen->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = glWinUnrealizeWindow;
    screen->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = glWinCopyWindow;

    {
      /* Dump out some useful information about the native renderer */
      HDC hdc;
      PIXELFORMATDESCRIPTOR pfd;
      int pixelFormat;
      HGLRC hglrc;

      // just use the screen DC
      hdc = GetDC(NULL);
      // we must set a pixel format before we can create a context, just use the one matching the first fbconfig
      fbConfigToPixelFormat(screen->base.fbconfigs, &pfd);
      pixelFormat = ChoosePixelFormat(hdc, &pfd);
      SetPixelFormat(hdc, pixelFormat, &pfd);
      hglrc = wglCreateContext(hdc);
      wglMakeCurrent(hdc, hglrc);

      ErrorF("GL_VERSION:    %s\n", glGetStringWrapperNonstatic(GL_VERSION));
      ErrorF("GL_VENDOR:     %s\n", glGetStringWrapperNonstatic(GL_VENDOR));
      ErrorF("GL_RENDERER:   %s\n", glGetStringWrapperNonstatic(GL_RENDERER));
      ErrorF("GL_EXTENSIONS: %s\n", glGetStringWrapperNonstatic(GL_EXTENSIONS));
      ErrorF("WGL_EXTENSIONS:%s\n", wglGetExtensionsStringARBWrapper(hdc));

      wglMakeCurrent(NULL, NULL);
      wglDeleteContext(hglrc);
      ReleaseDC(NULL, hdc);
    }

    // Based on the WGL extensions available, we might want to
    // override GLX version after __glXScreenInit() sets it to "1.2"
    // screen->base.GLXversion = xstrdup("1.4");
    /* We may wish to adjust screen->base.GLXextensions as well */
    // XXX: remove "GLX_SGIX_hyperpipe", "GLX_SGIX_swap_barrier"
    // XXX: remove "GLX_MESA_copy_sub_buffer" unless we implement it
    // __glXEnableExtension() ???

    return &screen->base;
}

/* ---------------------------------------------------------------------- */
/*
 * Window functions
 */

static Bool
glWinRealizeWindow(WindowPtr pWin)
{
    /* If this window has GL contexts, tell them to reattach */
    /* reattaching is bad: display lists and parameters get lost */
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glWinScreen *screenPriv = (glWinScreen *) glxGetScreen(pScreen);

    GLWIN_DEBUG_MSG("glWinRealizeWindow");

    /* Allow the window to be created (RootlessRealizeWindow is inside our wrap) */
    pScreen->RealizeWindow = screenPriv->RealizeWindow;
    result = pScreen->RealizeWindow(pWin);
    pScreen->RealizeWindow = glWinRealizeWindow;

#if 0
    __GLXdrawablePrivate *glxPriv;
    /* Re-attach this window's GL contexts, if any. */
    glxPriv = __glXFindDrawablePrivate(pWin->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;
        __GLcontext *gc;
        __GLdrawablePrivate *glPriv = &glxPriv->glPriv;
        GLWIN_DEBUG_MSG("glWinRealizeWindow is GL drawable!");

#if 0
        /* GL contexts bound to this window for drawing */
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            if (gc->isAttached)
#if 1
            {
                GLWIN_DEBUG_MSG("context is already bound! Adjusting HWND");
                glWinAdjustHWND(gc, pWin);
                continue;
            }
#else
                unattach(gc);
#endif
            attach(gc, glPriv);
        }

        /* GL contexts bound to this window for reading */
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            if (gc->isAttached)
#if 1
            {
                GLWIN_DEBUG_MSG("context is already bound! Adjusting HWND.");
                glWinAdjustHWND(gc, pWin);
                continue;
            }
#else
                unattach(gc);
#endif
            attach(gc, glPriv);
        }
#endif
    }
#endif

    return result;
}


static void
glWinCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    glWinScreen *screenPriv = (glWinScreen *) glxGetScreen(pScreen);

    GLWIN_TRACE_MSG("glWinCopyWindow pWindow %p", pWindow);

#if 0
    __GLXdrawablePrivate *glxPriv;
    /* Check if the window is attached and discard any drawing request */
    glxPriv = __glXFindDrawablePrivate(pWindow->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;

        /* GL contexts bound to this window for drawing */
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
/*
            GLWIN_DEBUG_MSG("glWinCopyWindow - calling glDrawBuffer");
            glDrawBuffer(GL_FRONT);
*/
            return;
        }

        /* GL contexts bound to this window for reading */
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            return;
        }
    }
#endif

    GLWIN_DEBUG_MSG("glWinCopyWindow - passing to hw layer");

    pScreen->CopyWindow = screenPriv->CopyWindow;
    pScreen->CopyWindow(pWindow, ptOldOrg, prgnSrc);
    pScreen->CopyWindow = glWinCopyWindow;
}

static Bool
glWinUnrealizeWindow(WindowPtr pWin)
{
    /* If this window has GL contexts, tell them to unattach */
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glWinScreen *screenPriv = (glWinScreen *)glxGetScreen(pScreen);

    GLWIN_DEBUG_MSG("glWinUnrealizeWindow");

#if 0
    __GLXdrawablePrivate *glxPriv;
    /* The native window may have already been destroyed (windows
     * are unrealized from top down)
     */

    /* Unattach this window's GL contexts, if any. */
    glxPriv = __glXFindDrawablePrivate(pWin->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;
        __GLcontext *gc;
        GLWIN_DEBUG_MSG("glWinUnrealizeWindow is GL drawable!");

#if 0
        /* GL contexts bound to this window for drawing */
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            unattach(gc);
        }

        /* GL contexts bound to this window for reading */
        for (gx = glxPriv->readGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            unattach(gc);
        }
#endif
    }
#endif

    pScreen->UnrealizeWindow = screenPriv->UnrealizeWindow;
    result = pScreen->UnrealizeWindow(pWin);
    pScreen->UnrealizeWindow = glWinUnrealizeWindow;

    return result;
}

/* ---------------------------------------------------------------------- */
/*
 * Drawable functions
 */

static GLboolean
glWinDrawableSwapBuffers(__GLXdrawable *base)
{
    HDC dc;
    HWND hwnd;
    BOOL ret;
    __GLXWinDrawable *draw = (__GLXWinDrawable *)base;

    /* Swap buffers on the last active context for drawing on the drawable */
    if (draw->context == NULL)
      {
        GLWIN_TRACE_MSG("glWinSwapBuffers - no context for drawable");
        return TRUE;
/*         return GL_FALSE; */
      }

    GLWIN_TRACE_MSG("glWinSwapBuffers on drawable %p, last context %p (native ctx %p)", base, draw->context, draw->context->ctx);

    /*
       draw->context->base.drawPriv will not be set if the context is not current anymore,
       but if it is, it should point to this drawable....
    */
    if (draw->context->base.drawPriv)
      {
        assert(draw->context->base.drawPriv == base);
      }

    dc = glWinMakeDC(draw->context, base, &dc, &hwnd);
    if (dc == NULL)
      return GL_FALSE;

    ret = wglSwapLayerBuffers(dc, WGL_SWAP_MAIN_PLANE);

    ReleaseDC(hwnd, dc);

    if (!ret)
      {
        ErrorF("wglSwapBuffers failed: %s\n", glWinErrorMessage());
        return GL_FALSE;
      }

    return GL_TRUE;
}

static void
glWinDrawableCopySubBuffer(__GLXdrawable *drawable,
                            int x, int y, int w, int h)
{
  ErrorF(" glWinDrawableCopySubBuffer: not implemented\n");
  /*TODO finish me*/
}

static void
glWinDrawableDestroy(__GLXdrawable *base)
{
  __GLXWinDrawable *glxPriv = (__GLXWinDrawable *)base;

  // XXX: really we need a list of all contexts pointing at this drawable, not just the last one...
/*   unattach(glxPriv->context); */
/*   glxPriv->context = NULL; */

  GLWIN_DEBUG_MSG("glWinDestroyDrawable");
  xfree(glxPriv);
}

static __GLXdrawable *
glWinCreateDrawable(__GLXscreen *screen,
                    DrawablePtr pDraw,
                    int type,
                    XID drawId,
                    __GLXconfig *conf)
{
  __GLXWinDrawable *glxPriv;

  glxPriv = xalloc(sizeof *glxPriv);

  if (glxPriv == NULL)
      return NULL;

  memset(glxPriv, 0, sizeof *glxPriv);

  if(!__glXDrawableInit(&glxPriv->base, screen, pDraw, type, drawId, conf)) {
    xfree(glxPriv);
    return NULL;
  }

  glxPriv->base.destroy       = glWinDrawableDestroy;
  glxPriv->base.swapBuffers   = glWinDrawableSwapBuffers;
  glxPriv->base.copySubBuffer = glWinDrawableCopySubBuffer;
  // glxPriv->base.waitX  what are these for?
  // glxPriv->base.waitGL

  GLWIN_DEBUG_MSG("glWinCreateDrawable %p", glxPriv);

  return &glxPriv->base;
}

/* ---------------------------------------------------------------------- */
/*
 * Texture functions
 */

// XXX: don't offer GLX_EXT_texture_from_pixmap if we can't implement these..

static
int glWinBindTexImage(__GLXcontext  *baseContext,
                      int            buffer,
                      __GLXdrawable *pixmap)
{
  ErrorF("glWinBindTexImage: not implemented\n");
  return FALSE;
}

static
int glWinReleaseTexImage(__GLXcontext  *baseContext,
                         int            buffer,
                         __GLXdrawable *pixmap)
{
  ErrorF(" glWinReleaseTexImage: not implemented\n");
  return FALSE;
}

/* ---------------------------------------------------------------------- */
/*
 * Lazy update context implementation
 *
 * WGL contexts are created for a specific HDC, so we cannot create the WGL
 * context in glWinCreateContext(), we must defer creation until the context
 * is actually used on a specifc drawable which is connected to a native window.
 *
 * The WGL context may be used on other, compatible HDCs, so we don't need to
 * recreate it for every new native window
 *
 * XXX: I wonder why we can't create the WGL context on the screen HDC ?
 */

static HDC
glWinMakeDC(__GLXWinContext *gc, __GLXdrawable *draw, HDC *hdc, HWND *hwnd)
{
  HDC dc;
  winWindowInfoRec winInfo;
  WindowPtr pWin;

  *hdc = NULL;
  *hwnd = NULL;

  if (draw == NULL)
    {
      GLWIN_TRACE_MSG("No drawable for context %p (native ctx %p)", gc, gc->ctx);
      return NULL;
    }

  pWin = (WindowPtr) draw->pDraw;
  if (pWin == NULL)
    {
      GLWIN_TRACE_MSG("for drawable %p, no WindowPtr", pWin);
      return NULL;
    }

  winGetWindowInfo(pWin, &winInfo);
  *hwnd = winInfo.hwnd;

  if (winInfo.hwnd == NULL)
    {
      ErrorF("No HWND error: %s\n", glWinErrorMessage());
      return NULL;
    }

  dc = GetDC(winInfo.hwnd);

  if (dc == NULL)
    ErrorF("GetDC error: %s\n", glWinErrorMessage());

  if (glWinDebugSettings.dumpDC)
      GLWIN_DEBUG_MSG("Got HDC %p", dc);

  /* Check if the hwnd has changed... */
  if (winInfo.hwnd != gc->hwnd)
    {
      int pixelFormat;

      if (glWinDebugSettings.enableTrace)
        GLWIN_DEBUG_HWND(winInfo.hwnd);

      GLWIN_TRACE_MSG("for context %p (native ctx %p), hWnd changed from %p to %p", gc, gc->ctx, gc->hwnd, winInfo.hwnd);
      gc->hwnd = winInfo.hwnd;

      /* We must select a pixelformat, but SetPixelFormat can only be called once for a window... */
      pixelFormat = ChoosePixelFormat(dc, &gc->pfd);
      if (pixelFormat == 0)
        {
          ErrorF("ChoosePixelFormat error: %s\n", glWinErrorMessage());
          return dc;
        }

      if (!SetPixelFormat(dc, pixelFormat, &gc->pfd))
        {
          ErrorF("SetPixelFormat error: %s\n", glWinErrorMessage());
          return dc;
        }
    }

  return dc;
}

static BOOL
glWinCreateContextReal(__GLXWinContext *gc, WindowPtr pWin)
{
    HDC dc;
    HWND hwnd;
    winWindowInfoRec winInfo;

    GLWIN_DEBUG_MSG("glWinCreateContextReal (pWin %p)", pWin);

    if (pWin == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until X window is created");
        return FALSE;
    }

    winGetWindowInfo(pWin, &winInfo);
    if (winInfo.hwnd == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until native window is created");
        return FALSE;
    }

    dc = glWinMakeDC(gc, gc->base.drawPriv, &dc, &hwnd);

    gc->ctx = wglCreateContext(dc);
    if (gc->ctx == NULL) {
        ErrorF("wglCreateContext error: %s\n", glWinErrorMessage());
        ReleaseDC(hwnd, dc);
        return FALSE;
    }

    GLWIN_DEBUG_MSG("glWinCreateContextReal (ctx %p)", gc->ctx);

    if (gc->shareContext)
      {
        GLWIN_DEBUG_MSG("glWinCreateContextReal shareLists with context%p (native ctx %p)", gc->shareContext, gc->shareContext->ctx);
        if (!wglShareLists(gc->shareContext->ctx, gc->ctx))
          {
            ErrorF("wglShareLists error: %s\n", glWinErrorMessage());
          }
      }

    if (!wglMakeCurrent(dc, gc->ctx)) {
        ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
        ReleaseDC(hwnd, dc);
        return FALSE;
    }

    ReleaseDC(hwnd, dc);

    return TRUE;
}

static void
attach(__GLXWinContext *gc, __GLXWinDrawable *draw)
{
    GLWIN_DEBUG_MSG("attach context %p to drawable %p", gc, draw);

    if (gc->isAttached)
    {
        ErrorF("called attach on an attached context\n");
        return;
    }

    if (draw->base.type == GLX_DRAWABLE_WINDOW)
    {
        WindowPtr pWin = (WindowPtr) draw->base.pDraw;
        if (pWin == NULL)
        {
          GLWIN_DEBUG_MSG("Deferring CreateContextReal() until window is created");
        }
        else
        {
          if (glWinCreateContextReal(gc, pWin))
            {
              gc->isAttached = TRUE;
              GLWIN_DEBUG_MSG("attached context %p to native context %p drawable %p", gc, gc->ctx, draw);
            }
        }
    }

    draw->context = gc;
}

static void
unattach(__GLXWinContext *gc)
{
    BOOL ret;
    GLWIN_DEBUG_MSG("unattach (ctx %p)", gc->ctx);

    if (!gc->isAttached)
    {
        ErrorF("called unattach on an unattached context\n");
        return;
    }

    if (gc->ctx)
    {
        ret = wglDeleteContext(gc->ctx);
        if (!ret)
            ErrorF("wglDeleteContext error: %s\n", glWinErrorMessage());
        gc->ctx = NULL;
    }

    gc->isAttached = 0;
}

/* ---------------------------------------------------------------------- */
/*
 * Context functions
 */


/* Context manipulation routines should return TRUE on success, FALSE on failure */
static int
glWinContextMakeCurrent(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;
  BOOL ret;
  HDC dc;
  HWND hwnd;

  GLWIN_TRACE_MSG("glWinContextMakeCurrent context %p (native ctx %p)", gc, gc->ctx);

  if (!gc->isAttached)
    attach(gc, (__GLXWinDrawable *)(gc->base.drawPriv));

  if (gc->ctx == NULL)
    {
      ErrorF("Native context is NULL\n");
      return FALSE;
    }

  dc = glWinMakeDC(gc, gc->base.drawPriv, &dc, &hwnd);
  if (dc == NULL)
    {
      ErrorF("glWinMakeDC failed\n");
      return FALSE;
    }

  // apparently this could fail if the context is current in a different thread,
  // but that shouldn't be able to happen in the current server...
  ret = wglMakeCurrent(dc, gc->ctx);
  if (!ret)
    ErrorF("wglMakeCurrent error: %s\n", glWinErrorMessage());
  ReleaseDC(hwnd, dc);

  return ret;
}

static int
glWinContextLoseCurrent(__GLXcontext *base)
{
  BOOL ret;
  __GLXWinContext *gc = (__GLXWinContext *)base;

  GLWIN_TRACE_MSG("glWinContextLoseCurrent context %p (native ctx %p)", gc, gc->ctx);
  glWinCallDelta();

  ret = wglMakeCurrent(NULL, NULL); /* We don't need a DC when setting no current context */
  if (!ret)
    ErrorF("glWinContextLoseCurrent error: %s\n", glWinErrorMessage());

  __glXLastContext = NULL; /* Mesa does this; why? */

  return TRUE;
}

static int
glWinContextCopy(__GLXcontext *dst_base, __GLXcontext *src_base, unsigned long mask)
{
  __GLXWinContext *dst = (__GLXWinContext *)dst_base;
  __GLXWinContext *src = (__GLXWinContext *)src_base;
  BOOL ret;

  GLWIN_DEBUG_MSG("glWinContextCopy");

  ret = wglCopyContext(src->ctx, dst->ctx, mask);
  if (!ret)
    {
      ErrorF("wglCopyContext error: %s\n", glWinErrorMessage());
    }

  return ret;
}

static int
glWinContextForceCurrent(__GLXcontext *base)
{
  /* wglMakeCurrent always flushes the previous context, so this is equivalent to glWinContextMakeCurrent */
  return glWinContextMakeCurrent(base);
}

static void
glWinContextDestroy(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;

  if (gc != NULL)
    {
      if (gc->isAttached)
        unattach(gc);

      GLWIN_DEBUG_MSG("GLXcontext %p destroyed (native ctx %p)", base, gc->ctx);

      xfree(gc);
    }
}

static __GLXcontext *
glWinCreateContext(__GLXscreen *screen,
                   __GLXconfig *modes,
                   __GLXcontext *baseShareContext)
{
    __GLXWinContext *context;
    __GLXWinContext *shareContext = (__GLXWinContext *)baseShareContext;

    static __GLXtextureFromPixmap glWinTextureFromPixmap =
      {
        glWinBindTexImage,
        glWinReleaseTexImage
      };

    context = (__GLXWinContext *)xcalloc(1, sizeof(__GLXWinContext));

    if (!context)
        return NULL;

    memset(context, 0, sizeof *context);
    context->base.destroy        = glWinContextDestroy;
    context->base.makeCurrent    = glWinContextMakeCurrent;
    context->base.loseCurrent    = glWinContextLoseCurrent;
    context->base.copy           = glWinContextCopy;
    context->base.forceCurrent   = glWinContextForceCurrent;
    context->base.textureFromPixmap = &glWinTextureFromPixmap;
    context->base.config = modes;
    context->base.pGlxScreen = screen;

    // actual native GL context creation is deferred until attach()
    // reason???
    context->ctx = NULL;
    context->isAttached = 0;
    context->shareContext = shareContext;

    // convert and store PFD
    if (fbConfigToPixelFormat(modes, &context->pfd))
    {
        ErrorF("glWinCreateContext: fbConfigToPixelFormat failed\n");
        xfree(context);
        return NULL;
    }

    if (glWinDebugSettings.dumpPFD)
        pfdOut(&context->pfd);

    setup_dispatch_table();

    GLWIN_DEBUG_MSG("GLXcontext %p created", context);

    return &(context->base);
}

/* ---------------------------------------------------------------------- */
/*
 * Utility functions
 */

static int
fbConfigToPixelFormat(__GLXconfig *mode, PIXELFORMATDESCRIPTOR *pfdret)
{
    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),   /* size of this pfd */
      1,                     /* version number */
      PFD_DRAW_TO_WINDOW |   /* support window */
      PFD_SUPPORT_OPENGL,    /* support OpenGL */
      PFD_TYPE_RGBA,         /* RGBA type */
      24,                    /* 24-bit color depth */
      0, 0, 0, 0, 0, 0,      /* color bits ignored */
      0,                     /* no alpha buffer */
      0,                     /* shift bit ignored */
      0,                     /* no accumulation buffer */
      0, 0, 0, 0,            /* accum bits ignored */
      32,                    /* 32-bit z-buffer */
      0,                     /* no stencil buffer */
      0,                     /* no auxiliary buffer */
      PFD_MAIN_PLANE,        /* main layer */
      0,                     /* reserved */
      0, 0, 0                /* layer masks ignored */
    };

    if (mode->stereoMode) {
        pfd.dwFlags |= PFD_STEREO;
    }
    if (mode->doubleBufferMode) {
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    }

    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = mode->redBits + mode->greenBits + mode->blueBits;
    pfd.cRedBits = mode->redBits;
    pfd.cRedShift = 0; /* FIXME */
    pfd.cGreenBits = mode->greenBits;
    pfd.cGreenShift = 0; /* FIXME  */
    pfd.cBlueBits = mode->blueBits;
    pfd.cBlueShift = 0; /* FIXME */
    pfd.cAlphaBits = mode->alphaBits;
    pfd.cAlphaShift = 0; /* FIXME */

    pfd.cAccumBits = mode->accumRedBits + mode->accumGreenBits + mode->accumBlueBits + mode->accumAlphaBits;
    pfd.cAccumRedBits = mode->accumRedBits;
    pfd.cAccumGreenBits = mode->accumGreenBits;
    pfd.cAccumBlueBits = mode->accumBlueBits;
    pfd.cAccumAlphaBits = mode->accumAlphaBits;

    pfd.cDepthBits = mode->depthBits;
    pfd.cStencilBits = mode->stencilBits;
    pfd.cAuxBuffers = mode->numAuxBuffers;

    if (mode->level > 0)
      {
        pfd.iLayerType = PFD_OVERLAY_PLANE;
      }
    else if (mode->level < 0)
      {
        pfd.iLayerType = PFD_UNDERLAY_PLANE;
      }

    /* mode->pixmapMode ? */

    *pfdret = pfd;

    return 0;
}

/* ---------------------------------------------------------------------- */

static __GLXconfig *
glWinCreateConfigs(int *numConfigsPtr, int screenNumber)
{
  __GLXconfig *c, *result;
  int numConfigs = 0;
  int i = 0;
  int n = 0;
  PIXELFORMATDESCRIPTOR pfd;
  HDC hdc;

  GLWIN_DEBUG_MSG("glWinCreateConfigs");

  // XXX: this is possibly wrong if we are not on the primary monitor...
  hdc = GetDC(NULL);

  // get the number of pixelformats
  numConfigs = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
  GLWIN_DEBUG_MSG("DescribePixelFormat says %d pixel formats", numConfigs);

  /* alloc */
  result = xalloc(sizeof(__GLXconfig) * numConfigs);

  if (NULL == result)
    {
      *numConfigsPtr = 0;
      return NULL;
    }

  memset(result, 0, sizeof(__GLXconfig) * numConfigs);
  c = result;
  n = 0;

  /* fill in configs */
  for (i = 0;  i < numConfigs; i++)
    {
      int rc;

      // point to next config, except for last
      if ((i + 1) < numConfigs)
        {
          c->next = c + 1;
        }
      else
        {
          c->next = NULL;
        }

      rc = DescribePixelFormat(hdc, i+1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

      if (!rc)
        {
          ErrorF("DescribePixelFormat failed for index %d, error %s\n", i+1, glWinErrorMessage());
          break;
        }

      if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL))
        {
          GLWIN_DEBUG_MSG("pixelFormat %d is unsuitable, skipping", i+1);
          continue;
        }

      if (glWinDebugSettings.dumpPFD)
        pfdOut(&pfd);

      c->doubleBufferMode = (pfd.dwFlags & PFD_DOUBLEBUFFER) ? GL_TRUE : GL_FALSE;
      c->stereoMode = (pfd.dwFlags & PFD_STEREO) ? GL_TRUE : GL_FALSE;

      c->redBits = pfd.cRedBits;
      c->greenBits = pfd.cGreenBits;
      c->blueBits = pfd.cBlueBits;
      c->alphaBits = pfd.cAlphaBits;

#define BITS_AND_SHIFT_TO_MASK(bits,mask) (((1<<(bits))-1) << (mask))

      c->redMask = BITS_AND_SHIFT_TO_MASK(pfd.cRedBits, pfd.cRedShift);
      c->greenMask =  BITS_AND_SHIFT_TO_MASK(pfd.cGreenBits, pfd.cGreenShift);
      c->blueMask = BITS_AND_SHIFT_TO_MASK(pfd.cBlueBits, pfd.cBlueShift);
      c->alphaMask = BITS_AND_SHIFT_TO_MASK(pfd.cAlphaBits, pfd.cAlphaShift);

      //      c->rgbBits = pfd.cColorBits;
      c->rgbBits = c->redBits + c->greenBits + c->blueBits + c->alphaBits;

      if (pfd.iPixelType == PFD_TYPE_COLORINDEX)
        {
          c->indexBits = pfd.cColorBits;
        }
      else
        {
          c->indexBits = 0;
        }

      c->accumRedBits = pfd.cAccumRedBits;
      c->accumGreenBits = pfd.cAccumGreenBits;
      c->accumBlueBits = pfd.cAccumBlueBits;
      c->accumAlphaBits = pfd.cAccumAlphaBits;
      //  pfd.cAccumBits;

      c->depthBits = pfd.cDepthBits;
      c->stencilBits = pfd.cStencilBits;
      c->numAuxBuffers = pfd.cAuxBuffers;

      // pfd.iLayerType; // ignored
      c->level = pfd.bReserved & 0xf; // overlay plane count, what to do with underlay plane count?
      // pfd.dwLayerMask; // ignored
      // pfd.dwDamageMask;  // ignored

      c->pixmapMode = 0;
      c->visualID = -1;  // will be set by __glXScreenInit()

      if (pfd.iPixelType == PFD_TYPE_COLORINDEX)
        {
          c->visualType = GLX_STATIC_COLOR;
        }
      else
        {
          c->visualType = GLX_TRUE_COLOR;
        }

      /* EXT_visual_rating / GLX 1.2 */
      // XXX: also consider PFD_GENERIC_ACCELERATED ?
      if (pfd.dwFlags & PFD_GENERIC_FORMAT)
        {
          c->visualRating = GLX_SLOW_CONFIG;
        }
      else
        {
          c->visualRating = GLX_NONE_EXT;
        }

      /* EXT_visual_info / GLX 1.2 */
      // pfd.dwVisibleMask; ???
      c->transparentPixel = GLX_NONE;
      c->transparentRed = GLX_NONE;
      c->transparentGreen = GLX_NONE;
      c->transparentBlue = GLX_NONE;
      c->transparentAlpha = GLX_NONE;
      c->transparentIndex = GLX_NONE;

      /* ARB_multisample / SGIS_multisample */
      c->sampleBuffers = 0;
      c->samples = 0;

      /* SGIX_fbconfig / GLX 1.3 */
      c->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
      c->renderType = GLX_RGBA_BIT;
      c->xRenderable = GL_TRUE;
      c->fbconfigID = -1; // will be set by __glXScreenInit()

      /* SGIX_pbuffer / GLX 1.3 */
      // XXX: How can we find these values out ???
      c->maxPbufferWidth = -1;
      c->maxPbufferHeight = -1;
      c->maxPbufferPixels = -1;
      c->optimalPbufferWidth = -1;
      c->optimalPbufferHeight = -1;

      /* SGIX_visual_select_group */
      c->visualSelectGroup = 0;

      /* OML_swap_method */
      c->swapMethod = GLX_SWAP_UNDEFINED_OML; // it may be that GLX_SWAP_EXCHANGE_OML is appropriate...

      c->screen = screenNumber;

      /* EXT_texture_from_pixmap */
      c->bindToTextureRgb = -1; // don't care is important for swrast to match fbConfigs
      c->bindToTextureRgba = -1;
      c->bindToMipmapTexture = -1;
      c->bindToTextureTargets = -1;
      c->yInverted = -1;

      n++;
      if (c->next)
        c = c->next;
    }

  ReleaseDC(NULL, hdc);

  GLWIN_DEBUG_MSG("found %d pixelFormats suitable for conversion to fbConfigs", n);
  *numConfigsPtr = n;
  return result;
}
