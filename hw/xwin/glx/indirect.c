/*
 * GLX implementation that uses Windows OpenGL library
 * (Indirect rendering path)
 *
 * Authors: Alexander Gottwald
 */
/*
 * Portions of this file are copied from GL/apple/indirect.c,
 * which contains the following copyright:
 *
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002 Apple Computer, Inc.
 *
 * Portions of this file are copied from xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#define WIN32_LEAN_AND_MEAN
#define _OBJC_NO_COM_ /* to prevent w32api basetyps.h defining 'interface' */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glwindows.h"
#include <glcontextmodes.h>
#include <stdint.h>

#include <winpriv.h>

#define GLWIN_DEBUG_HWND(hwnd)  \
    if (glWinDebugSettings.dumpHWND) { \
        char buffer[1024]; \
        if (GetWindowText(hwnd, buffer, sizeof(buffer))==0) *buffer=0; \
        GLWIN_DEBUG_MSG("Got HWND '%s' (%p)\n", buffer, hwnd); \
    }


//glWinDebugSettingsRec glWinDebugSettings = { 0, 0, 0, 0, 0, 0};
glWinDebugSettingsRec glWinDebugSettings = { 1, 1, 1, 1, 1, 0};

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

    envptr = getenv("GLWIN_ENABLE_STEREO");
    if (envptr != NULL)
	glWinDebugSettings.enableStereo = (atoi(envptr) == 1);
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
        snprintf(errorbuffer, sizeof(errorbuffer), "Unknown error in FormatMessage: %08x!\n", (unsigned)GetLastError());
    }
    return errorbuffer;
}

/*
 * GLX implementation that uses Win32's OpenGL
 */

/* Debug output */
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

static int
makeFormat(__GLXconfig *mode, PIXELFORMATDESCRIPTOR *pfdret)
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

    /* mode->level ignored */
    /* mode->pixmapMode ? */

    *pfdret = pfd;

    return 0;
}

/* ---------------------------------------------------------------------- */

struct __GLXWinContext {
  __GLXcontext base;
  HGLRC ctx;                         /* Windows GL Context */
  HDC dc;                            /* Windows Device Context */
  winWindowInfoRec winInfo;          /* Window info from XWin DDX */
  PIXELFORMATDESCRIPTOR pfd;         /* Pixel format descriptor */
  int pixelFormat;                   /* Pixel format index */
  unsigned isAttached :1;            /* Flag to track if context is attached */
};

typedef struct __GLXWinContext  __GLXWinContext;

struct __GLXWinDrawable
{
  __GLXdrawable base;
  // DrawablePtr pDraw;
  __GLXWinContext *context;
};

typedef struct __GLXWinDrawable __GLXWinDrawable;

/* ---------------------------------------------------------------------- */

static HDC
glWinMakeDC(__GLXWinContext *gc)
{
    HDC dc;

    if (glWinDebugSettings.enableTrace)
        GLWIN_DEBUG_HWND(gc->winInfo.hwnd);

    dc = GetDC(gc->winInfo.hwnd);

    if (dc == NULL)
        ErrorF("GetDC error: %s\n", glWinErrorMessage());
    return dc;
}

static BOOL
glWinCreateContextReal(__GLXWinContext *gc, WindowPtr pWin)
{
    HDC dc;
    BOOL ret;

    GLWIN_DEBUG_MSG("glWinCreateContextReal (pWin %p)\n", pWin);

    if (pWin == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until X window is created\n");
        return FALSE;
    }

    winGetWindowInfo(pWin, &gc->winInfo);

    GLWIN_DEBUG_HWND(gc->winInfo.hwnd);
    if (gc->winInfo.hwnd == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until native window is created\n");
        return FALSE;
    }

    dc = glWinMakeDC(gc);

    if (glWinDebugSettings.dumpDC)
        GLWIN_DEBUG_MSG("Got HDC %p\n", dc);

    gc->pixelFormat = ChoosePixelFormat(dc, &gc->pfd);
    if (gc->pixelFormat == 0)
    {
        ErrorF("ChoosePixelFormat error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }

    ret = SetPixelFormat(dc, gc->pixelFormat, &gc->pfd);
    if (!ret) {
        ErrorF("SetPixelFormat error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }

    gc->ctx = wglCreateContext(dc);
    if (gc->ctx == NULL) {
        ErrorF("wglCreateContext error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }

    GLWIN_DEBUG_MSG("glWinCreateContextReal (ctx %p)\n", gc->ctx);

    if (!wglMakeCurrent(dc, gc->ctx)) {
        ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }

    gc->dc = dc;
    //    ReleaseDC(gc->winInfo.hwnd, dc);

    return TRUE;
}

static void
attach(__GLXWinContext *gc, __GLXdrawable *glxPriv)
{
    GLWIN_DEBUG_MSG("attach (ctx %p)\n", gc->ctx);

    if (gc->isAttached)
    {
        ErrorF("called attach on an attached context\n");
        return;
    }

    if (glxPriv->type == GLX_DRAWABLE_WINDOW)
    {
        WindowPtr pWin = (WindowPtr) glxPriv->pDraw;
        if (pWin == NULL)
        {
            GLWIN_DEBUG_MSG("Deferring ChoosePixelFormat until window is created\n");
        }
        else
        {
            if (glWinCreateContextReal(gc, pWin))
              {
                gc->isAttached = TRUE;
                GLWIN_DEBUG_MSG("attached\n");
              }
        }
    }
}

static void
unattach(__GLXWinContext *gc)
{
    BOOL ret;
    GLWIN_DEBUG_MSG("unattach (ctx %p)\n", gc->ctx);

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

    if (gc->winInfo.hrgn)
    {
        ret = DeleteObject(gc->winInfo.hrgn);
        if (!ret)
            ErrorF("DeleteObject error: %s\n", glWinErrorMessage());
        gc->winInfo.hrgn = NULL;
    }

    gc->isAttached = 0;
}

/* ---------------------------------------------------------------------- */

/* Context manipulation routes return TRUE on success, FALSE on failure */

static int
glWinContextMakeCurrent(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;
  BOOL ret;
  HDC dc;

  GLWIN_TRACE_MSG("glWinContextMakeCurrent (ctx %p)\n", gc->ctx);

  if (!gc->isAttached)
    attach(gc, gc->base.drawPriv);

  if (gc->ctx == NULL) {
    ErrorF("Context is NULL\n");
    return FALSE;
  }

  //dc = glWinMakeDC(gc);
  dc = gc->dc;
  ret = wglMakeCurrent(dc, gc->ctx);
  if (!ret)
    ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
  //ReleaseDC(gc->winInfo.hwnd, dc);

  return ret;
}

static int
glWinContextLoseCurrent(__GLXcontext *base)
{
  HDC dc;
  BOOL ret;
  __GLXWinContext *gc = (__GLXWinContext *)base;
  GLWIN_TRACE_MSG("glWinLoseCurrent (ctx %p)\n", gc->ctx);

  dc = glWinMakeDC(gc);
  ret = wglMakeCurrent(dc, NULL);
  if (!ret)
    ErrorF("glMakeLoseCurrent error: %s\n", glWinErrorMessage());
  ReleaseDC(gc->winInfo.hwnd, dc);

  __glXLastContext = NULL; /* Mesa does this; why? */

  return TRUE;
}

static int
glWinContextCopy(__GLXcontext *dst_base, __GLXcontext *src_base, unsigned long mask)
{
  __GLXWinContext *dst = (__GLXWinContext *)dst_base;
  __GLXWinContext *src = (__GLXWinContext *)src_base;
  BOOL ret;

  GLWIN_DEBUG_MSG("glWinCopyContext\n");

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
  HDC dc;
  BOOL ret;
  __GLXWinContext *gc = (__GLXWinContext *)base;
  GLWIN_TRACE_MSG("glWinContextForceCurrent (ctx %p)\n", gc->ctx);

  dc = glWinMakeDC(gc);
  ret = wglMakeCurrent(dc, gc->ctx);
  if (!ret)
    ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
  ReleaseDC(gc->winInfo.hwnd, dc);

  return ret;
}

static void
glWinContextDestroy(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;
  GLWIN_DEBUG_MSG("glWinContextDestroy (ctx %p)\n", gc->ctx);

  if (gc != NULL)
    {
      if (gc->isAttached)
        unattach(gc);

      if (gc->dc != NULL)
        DeleteDC(gc->dc);

      xfree(gc);
    }
}

static __GLXcontext *
glWinCreateContext(__GLXscreen *screen,
                   __GLXconfig *modes,
                   __GLXcontext *shareContext)
{
    __GLXWinContext *context;
    // XXX: shareContext ???

    GLWIN_DEBUG_MSG("glWinCreateContext\n");

    context = (__GLXWinContext *)xcalloc(1, sizeof(__GLXWinContext));

    if (!context)
        return NULL;

    memset(context, 0, sizeof *context);
    context->base.destroy        = glWinContextDestroy;
    context->base.makeCurrent    = glWinContextMakeCurrent;
    context->base.loseCurrent    = glWinContextLoseCurrent;
    context->base.copy           = glWinContextCopy;
    context->base.forceCurrent   = glWinContextForceCurrent;
    context->base.config = modes;
    context->base.pGlxScreen = screen;

    // actual native GL context creation is deferred until attach()
    // reason???
    context->ctx = NULL;
    context->isAttached = 0;

    // convert and store PFD
    if (makeFormat(modes, &context->pfd))
    {
        ErrorF("makeFormat failed\n");
        xfree(context);
        return NULL;
    }

    if (glWinDebugSettings.dumpPFD)
        pfdOut(&context->pfd);

    setup_dispatch_table();

    GLWIN_DEBUG_MSG("glWinCreateContext done\n");

    return &(context->base);
}

static __GLXconfig *
glWinCreateConfigs(int *numConfigsPtr, int screenNumber)
{
  __GLXconfig *c, *result;
  int numConfigs = 0;
  int i = 0;

  GLWIN_DEBUG_MSG("glWinCreateConfigs\n");

  PIXELFORMATDESCRIPTOR pfd;
  HDC hdc;

  // XXX: this is possibly wrong if we are not on the primary monitor...
  hdc = GetDC(NULL);

  // get the number of pixelformats
  numConfigs = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
  GLWIN_DEBUG_MSG("DescribePixelFormat says %d configs\n", numConfigs);

  /* alloc */
  result = xalloc(sizeof(__GLXconfig) * numConfigs);

  if (NULL == result)
    {
      *numConfigsPtr = 0;
      return NULL;
    }

  memset(result, 0, sizeof(__GLXconfig) * numConfigs);
  c = result;

  /* fill in configs */
  for (i = 0;  i < numConfigs; i++)
    {
      // point to next config, except for last
      if ((i + 1) < numConfigs)
        {
          c->next = c + 1;
        }
      else
        {
          c->next = NULL;
        }

      int rc = DescribePixelFormat(hdc, i+1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

      if (!rc)
        {
          break;
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

      c->rgbBits = pfd.cColorBits;

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
      // pfd.bReserved;  // overlay/underlay planes
      // pfd.dwLayerMask; // ignored
      // pfd.dwDamageMask;  // ignored

      c->level = 0;
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

      // XXX: also consider PFD_GENERIC_ACCELERATED ?
      if (pfd.dwFlags & PFD_GENERIC_FORMAT)
        {
          c->visualRating = GLX_SLOW_CONFIG;
        }
      else
        {
          c->visualRating = GLX_NONE_EXT;
        }

      // pfd.dwVisibleMask; ???
      c->transparentPixel = GLX_NONE;
      c->transparentRed = GLX_NONE;
      c->transparentGreen = GLX_NONE;
      c->transparentBlue = GLX_NONE;
      c->transparentAlpha = GLX_NONE;
      c->transparentIndex = GLX_NONE;

      c->sampleBuffers = 0;
      c->samples = 0;

      c->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
      c->renderType = GLX_RGBA_BIT;
      c->xRenderable = GL_TRUE;
      c->fbconfigID = -1; // will be set by __glXScreenInit()

      /*
       * There is no introspection for this sort of thing
       * with CGL.  What should we do realistically?
       */
      c->optimalPbufferWidth = 0;
      c->optimalPbufferHeight = 0;
      c->visualSelectGroup = 0;
      c->swapMethod = GLX_SWAP_UNDEFINED_OML;
      c->screen = screenNumber;

      /* EXT_texture_from_pixmap */
      c->bindToTextureRgb = 0;
      c->bindToTextureRgba = 0;
      c->bindToMipmapTexture = 0;
      c->bindToTextureTargets = 0;
      c->yInverted = 0;

      if (c->next)
        c = c->next;
    }

  ReleaseDC(NULL, hdc);
  *numConfigsPtr = numConfigs;
  return result;
}

/* ---------------------------------------------------------------------- */

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

static Bool
glWinRealizeWindow(WindowPtr pWin)
{
    /* If this window has GL contexts, tell them to reattach */
    /* reattaching is bad: display lists and parameters get lost */
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glWinScreen *screenPriv = (glWinScreen *) glxGetScreen(pScreen);

    GLWIN_DEBUG_MSG("glWinRealizeWindow\n");

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
        GLWIN_DEBUG_MSG("glWinRealizeWindow is GL drawable!\n");

#if 0
        /* GL contexts bound to this window for drawing */
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
            gc = (__GLcontext *)gx->gc;
            if (gc->isAttached)
#if 1
            {
                GLWIN_DEBUG_MSG("context is already bound! Adjusting HWND.\n");
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
                GLWIN_DEBUG_MSG("context is already bound! Adjusting HWND.\n");
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

    GLWIN_TRACE_MSG(" (pWindow %p)\n", pWindow);

#if 0
    __GLXdrawablePrivate *glxPriv;
    /* Check if the window is attached and discard any drawing request */
    glxPriv = __glXFindDrawablePrivate(pWindow->drawable.id);
    if (glxPriv) {
        __GLXcontext *gx;

        /* GL contexts bound to this window for drawing */
        for (gx = glxPriv->drawGlxc; gx != NULL; gx = gx->next) {
/*
            GLWIN_DEBUG_MSG("glWinCopyWindow - calling glDrawBuffer\n");
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

    GLWIN_DEBUG_MSG("glWinCopyWindow - passing to hw layer\n");

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

    GLWIN_DEBUG_MSG("glWinUnrealizeWindow\n");

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
        GLWIN_DEBUG_MSG("glWinUnrealizeWindow is GL drawable!\n");

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

static void
glWinScreenDestroy(__GLXscreen *screen)
{
    GLWIN_DEBUG_MSG("glWinScreenDestroy(%p)\n", screen);
    __glXScreenDestroy(screen);
    xfree(screen);
}

static GLboolean
glWinDrawableSwapBuffers(__GLXdrawable *base)
{
  /* swap buffers on only *one* of the contexts
   * (e.g. the last one for drawing)
   */
    HDC dc;
    BOOL ret;

    GLWIN_TRACE_MSG("glWinSwapBuffers (drawable %p, context %p)\n", base, wglGetCurrentContext()) ;

    dc = wglGetCurrentDC();
    if (dc == NULL)
      return GL_FALSE;

    //    ret = SwapBuffers(dc);
    //    if (!ret)
    //      ErrorF("SwapBuffers failed: %s\n", glWinErrorMessage());

    ret = wglSwapLayerBuffers(dc, WGL_SWAP_MAIN_PLANE);
    if (!ret)
      ErrorF("wglSwapBuffers failed: %s\n", glWinErrorMessage());

    //    ReleaseDC(gc->winInfo.hwnd, dc);
    // if (!ret)
    // return GL_FALSE;
    return GL_TRUE;
}

static void
glWinDrawableCopySubBuffer(__GLXdrawable *drawable,
                            int x, int y, int w, int h)
{
  /*TODO finish me*/
}

static void
glWinDrawableDestroy(__GLXdrawable *base)
{
  __GLXWinDrawable *glxPriv = (__GLXWinDrawable *)base;
  GLWIN_DEBUG_MSG("glWinDestroyDrawable\n");
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
  GLWIN_DEBUG_MSG("glWinCreateDrawable\n");

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

  // glxPriv->pDraw = pDraw;
  // glxPriv->sid = 0;
  // glxPriv->context = NULL;

  return &glxPriv->base;
}

/* ---------------------------------------------------------------------- */

/* This is called by GlxExtensionInit() */
static __GLXscreen *
glWinScreenProbe(ScreenPtr pScreen)
{
    glWinScreen *screen;

    GLWIN_DEBUG_MSG("glWinScreenProbe\n");

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

    /* Do we want to override these after __glXScreenInit() has set them? */
    screen->base.GLXversion = xstrdup("1.4");
    screen->base.GLXextensions = xstrdup("");
    /* We may be able to add more GLXextensions at a later time. */

    return &screen->base;
}

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
