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

typedef struct __GLXdrawablePrivateRec __GLXdrawablePrivate;

// copied from old GLXdrawable.h
struct __GLXdrawablePrivateRec
{
    DrawablePtr pDraw;
   /*
    ** Either DRAWABLE_PIXMAP or DRAWABLE_WINDOW, copied from pDraw above.
    ** Needed by the resource freer because pDraw might already have been
    ** freed.
    */
    int type;

    /*
    ** Lists of contexts bound to this drawable.  There are two lists here.
    ** One list is of the contexts that have this drawable bound for drawing,
    ** and the other is the list of contexts that have this drawable bound
    ** for reading.
    */
    struct __GLXcontext *drawGlxc;
    struct __GLXcontext *readGlxc;

    /*
    ** The GL drawable (information shared between GLX and the GL core)
    */
    __GLdrawablePrivate glPriv;
};

// copied from old glxutil.c
__GLXdrawablePrivate *__glXFindDrawablePrivate(XID drawId);

#define GLWIN_DEBUG_HWND(hwnd)  \
    if (glWinDebugSettings.dumpHWND) { \
        char buffer[1024]; \
        if (GetWindowText(hwnd, buffer, sizeof(buffer))==0) *buffer=0; \
        GLWIN_DEBUG_MSG("Got HWND %s (%p)\n", buffer, hwnd); \
    }


glWinDebugSettingsRec glWinDebugSettings = { 0, 0, 0, 0, 0, 0};

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

/*
 * Server-side GLX uses these functions which are normally defined
 * in the OpenGL SI.
 */

static
GLuint __glFloorLog2(GLuint val)
{
    int c = 0;

    while (val > 1) {
        c++;
        val >>= 1;
    }
    return c;
}

/* some prototypes */
static  __GLXscreen *glWinScreenProbe(ScreenPtr pScreen);
#if 0
static Bool glWinInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
                              int *nvisualp, int *ndepthp,
                              int *rootDepthp, VisualID *defaultVisp,
                              unsigned long sizes, int bitsPerRGB);
static void glWinSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
                                   void **privates);
static __GLinterface *glWinCreateContext(__GLimports *imports,
                                          __GLcontextModes *mode,
                                          __GLinterface *shareGC);
static void glWinCreateBuffer(__GLXdrawablePrivate *glxPriv);
static void glWinResetExtension(void);
#endif

#if 0
/*
 * This structure is statically allocated in the __glXScreens[]
 * structure.  This struct is not used anywhere other than in
 * __glXScreenInit to initialize each of the active screens
 * (__glXActiveScreens[]).  Several of the fields must be initialized by
 * the screenProbe routine before they are copied to the active screens
 * struct.  In particular, the contextCreate, pGlxVisual, numVisuals,
 * and numUsableVisuals fields must be initialized.
 */
static __GLXscreenInfo __glDDXScreenInfo = {
    glWinScreenProbe,   /* Must be generic and handle all screens */
    glWinCreateContext, /* Substitute screen's createContext routine */
    glWinCreateBuffer,  /* Substitute screen's createBuffer routine */
    NULL,                 /* Set up pGlxVisual in probe */
    NULL,                 /* Set up pVisualPriv in probe */
    0,                    /* Set up numVisuals in probe */
    0,                    /* Set up numUsableVisuals in probe */
    NULL,                 /* GLextensions is overwritten by __glXScreenInit */
    "Vendor String",      /* GLXvendor is overwritten by __glXScreenInit */
    "Version String",     /* GLXversion is overwritten by __glXScreenInit */
    "Extensions String",  /* GLXextensions is overwritten by __glXScreenInit */
    NULL                  /* WrappedPositionWindow is overwritten */
};

void *__glXglDDXScreenInfo(void) {
    return &__glDDXScreenInfo;
}
#endif

#if 0
static __GLXextensionInfo __glDDXExtensionInfo = {
    GL_CORE_WINDOWS,
    glWinResetExtension,
    glWinInitVisuals,
    glWinSetVisualConfigs
};

void *__glXglDDXExtensionInfo(void) {
    return &__glDDXExtensionInfo;
}
#endif

/* prototypes */

static GLboolean glWinDestroyContext(__GLcontext *gc);
static GLboolean glWinLoseCurrent(__GLcontext *gc);
static GLboolean glWinMakeCurrent(__GLcontext *gc);
static GLboolean glWinShareContext(__GLcontext *gc, __GLcontext *gcShare);
static GLboolean glWinCopyContext(__GLcontext *dst, const __GLcontext *src,
                            GLuint mask);
static GLboolean glWinForceCurrent(__GLcontext *gc);

/* Drawing surface notification callbacks */
static GLboolean glWinNotifyResize(__GLcontext *gc);
static void glWinNotifyDestroy(__GLcontext *gc);
static void glWinNotifySwapBuffers(__GLcontext *gc);

/* Dispatch table override control for external agents like libGLS */
static struct __GLdispatchStateRec* glWinDispatchExec(__GLcontext *gc);
static void glWinBeginDispatchOverride(__GLcontext *gc);
static void glWinEndDispatchOverride(__GLcontext *gc);

/* Debug output */
static void pfdOut(const PIXELFORMATDESCRIPTOR *pfd);

static __GLexports glWinExports = {
    glWinDestroyContext,
    glWinLoseCurrent,
    glWinMakeCurrent,
    glWinShareContext,
    glWinCopyContext,
    glWinForceCurrent,

    glWinNotifyResize,
    glWinNotifyDestroy,
    glWinNotifySwapBuffers,

    glWinDispatchExec,
    glWinBeginDispatchOverride,
    glWinEndDispatchOverride
};

glWinScreenRec glWinScreens[MAXSCREENS];

/* __GLdrawablePrivate->private */
typedef struct {
    DrawablePtr pDraw;
  /*    xp_surface_id sid; */
} GLWinDrawableRec;

struct __GLcontextRec {
  struct __GLinterfaceRec interface; /* required to be first */

  HGLRC ctx;                         /* Windows GL Context */
  
  HDC dc;                            /* Windows Device Context */
  winWindowInfoRec winInfo;          /* Window info from XWin */
  
  PIXELFORMATDESCRIPTOR pfd;         /* Pixelformat flags */
  int pixelFormat;                   /* Pixelformat index */

  unsigned isAttached :1;            /* Flag to track if context is attached */
};

static HDC glWinMakeDC(__GLcontext *gc)
{
    HDC dc;

    /*if (gc->winInfo.hrgn == NULL) 
    {
        GLWIN_DEBUG_MSG("Creating region from RECT(%ld,%ld,%ld,%ld):",
                gc->winInfo.rect.left,
                gc->winInfo.rect.top,
                gc->winInfo.rect.right,
                gc->winInfo.rect.bottom);
        gc->winInfo.hrgn = CreateRectRgnIndirect(&gc->winInfo.rect);
        GLWIN_DEBUG_MSG2("%p\n", gc->winInfo.hrgn);
    }*/

    if (glWinDebugSettings.enableTrace)
        GLWIN_DEBUG_HWND(gc->winInfo.hwnd);

    dc = GetDC(gc->winInfo.hwnd); 
    /*dc = GetDCEx(gc->winInfo.hwnd, gc->winInfo.hrgn, 
            DCX_WINDOW | DCX_NORESETATTRS ); */

    if (dc == NULL)
        ErrorF("GetDC error: %s\n", glWinErrorMessage());
    return dc;
}

static void unattach(__GLcontext *gc)
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

static BOOL glWinAdjustHWND(__GLcontext *gc, WindowPtr pWin)
{
    HDC dc;
    BOOL ret;
    HGLRC newctx;
    HWND oldhwnd;

    GLWIN_DEBUG_MSG("glWinAdjustHWND (ctx %p, pWin %p)\n", gc->ctx, pWin);

    if (pWin == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until window is created\n");
        return FALSE;
    }

    oldhwnd = gc->winInfo.hwnd;
    winGetWindowInfo(pWin, &gc->winInfo);
    
    GLWIN_DEBUG_HWND(gc->winInfo.hwnd);
    if (gc->winInfo.hwnd == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until window is created\n");
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
    
    newctx = wglCreateContext(dc);
    if (newctx == NULL) {
        ErrorF("wglCreateContext error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }
    
    GLWIN_DEBUG_MSG("wglCreateContext (ctx %p)\n", newctx);

    if (!wglShareLists(gc->ctx, newctx))
    {
        ErrorF("wglShareLists error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }
    
    if (oldhwnd != gc->winInfo.hwnd)
    {
        GLWIN_DEBUG_MSG("Trying wglCopyContext\n");
        if (!wglCopyContext(gc->ctx, newctx, GL_ALL_ATTRIB_BITS))
        {
            ErrorF("wglCopyContext error: %s\n", glWinErrorMessage());
            ReleaseDC(gc->winInfo.hwnd, dc);
            return FALSE;
        }
    }

    if (!wglDeleteContext(gc->ctx))
    {
        ErrorF("wglDeleteContext error: %s\n", glWinErrorMessage());
    }

    gc->ctx = newctx;

    if (!wglMakeCurrent(dc, gc->ctx)) {
        ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
        ReleaseDC(gc->winInfo.hwnd, dc);
        return FALSE;
    }

    ReleaseDC(gc->winInfo.hwnd, dc);

    return TRUE;
}

static BOOL glWinCreateContextReal(__GLcontext *gc, WindowPtr pWin)
{
    HDC dc;
    BOOL ret;

    GLWIN_DEBUG_MSG("glWinCreateContextReal (pWin %p)\n", pWin);

    if (pWin == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until window is created\n");
        return FALSE;
    }

    winGetWindowInfo(pWin, &gc->winInfo);
    
    GLWIN_DEBUG_HWND(gc->winInfo.hwnd);
    if (gc->winInfo.hwnd == NULL)
    {
        GLWIN_DEBUG_MSG("Deferring until window is created\n");
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

    ReleaseDC(gc->winInfo.hwnd, dc);

    return TRUE;
}

static void attach(__GLcontext *gc, __GLdrawablePrivate *glPriv)
{
    __GLXdrawablePrivate *glxPriv = (__GLXdrawablePrivate *)glPriv->other;

    GLWIN_DEBUG_MSG("attach (ctx %p)\n", gc->ctx);

    if (gc->isAttached)
    {
        ErrorF("called attach on an attached context\n");
        return;
    }

    if (glxPriv->type == DRAWABLE_WINDOW)
    {
        WindowPtr pWin = (WindowPtr) glxPriv->pDraw;
        if (pWin == NULL)
        {
            GLWIN_DEBUG_MSG("Deferring ChoosePixelFormat until window is created\n");
        } else
        {
            if (glWinCreateContextReal(gc, pWin))
            {
                gc->isAttached = TRUE;
                GLWIN_DEBUG_MSG("attached\n");
            }
        }
    }
}

static GLboolean glWinLoseCurrent(__GLcontext *gc)
{
    GLWIN_TRACE_MSG("glWinLoseCurrent (ctx %p)\n", gc->ctx);

    __glXLastContext = NULL; /* Mesa does this; why? */

    return GL_TRUE;
}

/* Context manipulation; return GL_FALSE on failure */
static GLboolean glWinDestroyContext(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("glWinDestroyContext (ctx %p)\n", gc->ctx);

    if (gc != NULL)
    {
        if (gc->isAttached)
            unattach(gc);
        if (gc->dc != NULL)
            DeleteDC(gc->dc);
        free(gc);
    }

    return GL_TRUE;
}

static GLboolean glWinMakeCurrent(__GLcontext *gc)
{
    __GLdrawablePrivate *glPriv = gc->interface.imports.getDrawablePrivate(gc);
    BOOL ret;
    HDC dc;

    GLWIN_TRACE_MSG(" (ctx %p)\n", gc->ctx);

    if (!gc->isAttached)
        attach(gc, glPriv);

    if (gc->ctx == NULL) {
        ErrorF("Context is NULL\n");
        return GL_FALSE;
    }

    dc = glWinMakeDC(gc);
    ret = wglMakeCurrent(dc, gc->ctx);
    if (!ret)
        ErrorF("glMakeCurrent error: %s\n", glWinErrorMessage());
    ReleaseDC(gc->winInfo.hwnd, dc);

    return ret?GL_TRUE:GL_FALSE;
}

static GLboolean glWinShareContext(__GLcontext *gc, __GLcontext *gcShare)
{
  GLWIN_DEBUG_MSG("glWinShareContext unimplemented\n");

  return GL_TRUE;
}

static GLboolean glWinCopyContext(__GLcontext *dst, const __GLcontext *src,
                                   GLuint mask)
{
    BOOL ret;

    GLWIN_DEBUG_MSG("glWinCopyContext\n");
    
    ret = wglCopyContext(src->ctx, dst->ctx, mask);
    if (!ret) 
    {
        ErrorF("wglCopyContext error: %s\n", glWinErrorMessage());
        return GL_FALSE;
    }

    return GL_TRUE;
}

static GLboolean glWinForceCurrent(__GLcontext *gc)
{
    GLWIN_TRACE_MSG(" (ctx %p)\n", gc->ctx);

    return GL_TRUE;
}

/* Drawing surface notification callbacks */

static GLboolean glWinNotifyResize(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinNotifyResize");
    return GL_TRUE;
}

static void glWinNotifyDestroy(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinNotifyDestroy");
}

static void glWinNotifySwapBuffers(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinNotifySwapBuffers");
}

/* Dispatch table override control for external agents like libGLS */
static struct __GLdispatchStateRec* glWinDispatchExec(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinDispatchExec");
    return NULL;
}

static void glWinBeginDispatchOverride(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinBeginDispatchOverride");
}

static void glWinEndDispatchOverride(__GLcontext *gc)
{
    GLWIN_DEBUG_MSG("unimplemented glWinEndDispatchOverride");
}

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

static int makeFormat(__GLcontextModes *mode, PIXELFORMATDESCRIPTOR *pfdret)
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
    }, *result = &pfd;

    /* disable anything but rgba. must get rgba to work first */
    if (!mode->rgbMode) 
        return -1; 
    
    if (mode->stereoMode) {
        result->dwFlags |= PFD_STEREO;
    }
    if (mode->doubleBufferMode) {
        result->dwFlags |= PFD_DOUBLEBUFFER;
    }

    if (mode->colorIndexMode) {
        /* ignored, see above */
        result->iPixelType = PFD_TYPE_COLORINDEX; 
        result->cColorBits = mode->redBits + mode->greenBits + mode->blueBits;
        result->cRedBits = mode->redBits;
        result->cRedShift = 0; /* FIXME */
        result->cGreenBits = mode->greenBits;
        result->cGreenShift = 0; /* FIXME  */
        result->cBlueBits = mode->blueBits;
        result->cBlueShift = 0; /* FIXME */
        result->cAlphaBits = mode->alphaBits;
        result->cAlphaShift = 0; /* FIXME */
    }

    if (mode->rgbMode) {
        result->iPixelType = PFD_TYPE_RGBA;
        result->cColorBits = mode->redBits + mode->greenBits + mode->blueBits;
        result->cRedBits = mode->redBits;
        result->cRedShift = 0; /* FIXME */
        result->cGreenBits = mode->greenBits;
        result->cGreenShift = 0; /* FIXME  */
        result->cBlueBits = mode->blueBits;
        result->cBlueShift = 0; /* FIXME */
        result->cAlphaBits = mode->alphaBits;
        result->cAlphaShift = 0; /* FIXME */
    }

    if (mode->haveAccumBuffer) {
        result->cAccumBits = mode->accumRedBits + mode->accumGreenBits
            + mode->accumBlueBits + mode->accumAlphaBits;
        result->cAccumRedBits = mode->accumRedBits;
        result->cAccumGreenBits = mode->accumGreenBits;
        result->cAccumBlueBits = mode->accumBlueBits;
        result->cAccumAlphaBits = mode->accumAlphaBits;
    }
    
    if (mode->haveDepthBuffer) {
        result->cDepthBits = mode->depthBits;
    }
    if (mode->haveStencilBuffer) {
        result->cStencilBits = mode->stencilBits;
    }

    /* result->cAuxBuffers = mode->numAuxBuffers; */

    /* mode->level ignored */

    /* mode->pixmapMode ? */

    *pfdret = pfd;

    return 0;
}

#if 0
static __GLinterface *glWinCreateContext(__GLimports *imports,
                                          __GLcontextModes *mode,
                                          __GLinterface *shareGC)
{
    __GLcontext *result;

    GLWIN_DEBUG_MSG("glWinCreateContext\n");

    result = (__GLcontext *)calloc(1, sizeof(__GLcontext));
    if (!result) 
        return NULL;

    result->interface.imports = *imports;
    result->interface.exports = glWinExports;

    if (makeFormat(mode, &result->pfd))
    {
        ErrorF("makeFormat failed\n");
        free(result);
        return NULL;
    }

    if (glWinDebugSettings.dumpPFD)
        pfdOut(&result->pfd);

    GLWIN_DEBUG_MSG("glWinCreateContext done\n");
    return (__GLinterface *)result;
}
#endif

static
Bool
glWinRealizeWindow(WindowPtr pWin)
{
    /* If this window has GL contexts, tell them to reattach */
    /* reattaching is bad: display lists and parameters get lost */
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glWinScreenRec *screenPriv = &glWinScreens[pScreen->myNum];
    __GLXdrawablePrivate *glxPriv;

    GLWIN_DEBUG_MSG("glWinRealizeWindow\n");

    /* Allow the window to be created (RootlessRealizeWindow is inside our wrap) */
    pScreen->RealizeWindow = screenPriv->RealizeWindow;
    result = pScreen->RealizeWindow(pWin);
    pScreen->RealizeWindow = glWinRealizeWindow;

#if 0
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
    glWinScreenRec *screenPriv = &glWinScreens[pScreen->myNum];
    __GLXdrawablePrivate *glxPriv;

    GLWIN_TRACE_MSG(" (pWindow %p)\n", pWindow);

#if 0
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
    glWinScreenRec *screenPriv = &glWinScreens[pScreen->myNum];
    __GLXdrawablePrivate *glxPriv;

    GLWIN_DEBUG_MSG("glWinUnrealizeWindow\n");

#if 0
    /* The Aqua window may have already been destroyed (windows
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


/* Originally copied from Mesa */

static int                 numConfigs     = 0;
static __GLXvisualConfig  *visualConfigs  = NULL;
static void              **visualPrivates = NULL;

/*
 * In the case the driver defines no GLX visuals we'll use these.
 * Note that for TrueColor and DirectColor visuals, bufferSize is the
 * sum of redSize, greenSize, blueSize and alphaSize, which may be larger
 * than the nplanes/rootDepth of the server's X11 visuals
 */
#define NUM_FALLBACK_CONFIGS 5
static __GLXvisualConfig FallbackConfigs[NUM_FALLBACK_CONFIGS] = {
  /* [0] = RGB, double buffered, Z */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [1] = RGB, double buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
    16, 16, 16, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [2] = RGB+Alpha, double buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [3] = RGB+Alpha, single buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    False,              /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [4] = CI, double buffered, Z */
  {
    -1,                 /* vid */
    -1,                 /* class */
    False,              /* rgba? (false = color index) */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
};

static __GLXvisualConfig NullConfig = {
    -1,                 /* vid */
    -1,                 /* class */
    False,              /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    False,              /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE_EXT,       /* visualRating */
    0,                  /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
};

static inline int count_bits(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 63;
}

/* Mostly copied from Mesa's xf86glx.c */
static Bool init_visuals(int *nvisualp, VisualPtr *visualp,
                         VisualID *defaultVisp,
                         int ndepth, DepthPtr pdepth,
                         int rootDepth)
{
    int numRGBconfigs;
    int numCIconfigs;
    int numVisuals = *nvisualp;
    int numNewVisuals;
    int numNewConfigs;
    VisualPtr pVisual = *visualp;
    VisualPtr pVisualNew = NULL;
    VisualID *orig_vid = NULL;
    __GLcontextModes *modes;
    __GLXvisualConfig *pNewVisualConfigs = NULL;
    void **glXVisualPriv;
    void **pNewVisualPriv;
    int found_default;
    int i, j, k;

    GLWIN_DEBUG_MSG("init_visuals\n");

    if (numConfigs > 0)
        numNewConfigs = numConfigs;
    else
        numNewConfigs = NUM_FALLBACK_CONFIGS;

    /* Alloc space for the list of new GLX visuals */
    pNewVisualConfigs = (__GLXvisualConfig *)
                     malloc(numNewConfigs * sizeof(__GLXvisualConfig));
    if (!pNewVisualConfigs) {
        return FALSE;
    }

    /* Alloc space for the list of new GLX visual privates */
    pNewVisualPriv = (void **) malloc(numNewConfigs * sizeof(void *));
    if (!pNewVisualPriv) {
        free(pNewVisualConfigs);
        return FALSE;
    }

    /*
    ** If SetVisualConfigs was not called, then use default GLX
    ** visual configs.
    */
    if (numConfigs == 0) {
        memcpy(pNewVisualConfigs, FallbackConfigs,
               NUM_FALLBACK_CONFIGS * sizeof(__GLXvisualConfig));
        memset(pNewVisualPriv, 0, NUM_FALLBACK_CONFIGS * sizeof(void *));
    }
    else {
        /* copy driver's visual config info */
        for (i = 0; i < numConfigs; i++) {
            pNewVisualConfigs[i] = visualConfigs[i];
            pNewVisualPriv[i] = visualPrivates[i];
        }
    }

    /* Count the number of RGB and CI visual configs */
    numRGBconfigs = 0;
    numCIconfigs = 0;
    for (i = 0; i < numNewConfigs; i++) {
        if (pNewVisualConfigs[i].rgba)
            numRGBconfigs++;
        else
            numCIconfigs++;
    }

    /* Count the total number of visuals to compute */
    numNewVisuals = 0;
    for (i = 0; i < numVisuals; i++) {
        numNewVisuals +=
            (pVisual[i].class == TrueColor || pVisual[i].class == DirectColor)
            ? numRGBconfigs : numCIconfigs;
    }

    /* Reset variables for use with the next screen/driver's visual configs */
    visualConfigs = NULL;
    numConfigs = 0;

    /* Alloc temp space for the list of orig VisualIDs for each new visual */
    orig_vid = (VisualID *)malloc(numNewVisuals * sizeof(VisualID));
    if (!orig_vid) {
        free(pNewVisualPriv);
        free(pNewVisualConfigs);
        return FALSE;
    }

    /* Alloc space for the list of glXVisuals */
    modes = _gl_context_modes_create(numNewVisuals, sizeof(__GLcontextModes));
    if (modes == NULL) {
        free(orig_vid);
        free(pNewVisualPriv);
        free(pNewVisualConfigs);
        return FALSE;
    }

    /* Alloc space for the list of glXVisualPrivates */
    glXVisualPriv = (void **)malloc(numNewVisuals * sizeof(void *));
    if (!glXVisualPriv) {
        _gl_context_modes_destroy( modes );
        free(orig_vid);
        free(pNewVisualPriv);
        free(pNewVisualConfigs);
        return FALSE;
    }

    /* Alloc space for the new list of the X server's visuals */
    pVisualNew = (VisualPtr)malloc(numNewVisuals * sizeof(VisualRec));
    if (!pVisualNew) {
        free(glXVisualPriv);
        _gl_context_modes_destroy( modes );
        free(orig_vid);
        free(pNewVisualPriv);
        free(pNewVisualConfigs);
        return FALSE;
    }

    /* Initialize the new visuals */
    found_default = FALSE;
    glWinScreens[screenInfo.numScreens-1].modes = modes;
    for (i = j = 0; i < numVisuals; i++) {
        int is_rgb = (pVisual[i].class == TrueColor ||
                      pVisual[i].class == DirectColor);

        if (!is_rgb)
        {
            /* We don't support non-rgb visuals for GL. But we don't
               want to remove them either, so just pass them through
               with null glX configs */

            pVisualNew[j] = pVisual[i];
            pVisualNew[j].vid = FakeClientID(0);

            /* Check for the default visual */
            if (!found_default && pVisual[i].vid == *defaultVisp) {
                *defaultVisp = pVisualNew[j].vid;
                found_default = TRUE;
            }

            /* Save the old VisualID */
            orig_vid[j] = pVisual[i].vid;

            /* Initialize the glXVisual */
            _gl_copy_visual_to_context_mode( modes, & NullConfig );
            modes->visualID = pVisualNew[j].vid;

            j++;

            continue;
        }

        for (k = 0; k < numNewConfigs; k++) {
            if (pNewVisualConfigs[k].rgba != is_rgb)
                continue;

            assert( modes != NULL );

            /* Initialize the new visual */
            pVisualNew[j] = pVisual[i];
            pVisualNew[j].vid = FakeClientID(0);

            /* Check for the default visual */
            if (!found_default && pVisual[i].vid == *defaultVisp) {
                *defaultVisp = pVisualNew[j].vid;
                found_default = TRUE;
            }

            /* Save the old VisualID */
            orig_vid[j] = pVisual[i].vid;

            /* Initialize the glXVisual */
            _gl_copy_visual_to_context_mode( modes, & pNewVisualConfigs[k] );
            modes->visualID = pVisualNew[j].vid;
	    if (modes->fbconfigID == GLX_DONT_CARE)
		modes->fbconfigID = modes->visualID;

            /*
             * If the class is -1, then assume the X visual information
             * is identical to what GLX needs, and take them from the X
             * visual.  NOTE: if class != -1, then all other fields MUST
             * be initialized.
             */
            if (modes->visualType == GLX_NONE) {
                modes->visualType = _gl_convert_from_x_visual_type( pVisual[i].class );
                modes->redBits    = count_bits(pVisual[i].redMask);
                modes->greenBits  = count_bits(pVisual[i].greenMask);
                modes->blueBits   = count_bits(pVisual[i].blueMask);
                modes->alphaBits  = modes->alphaBits;
                modes->redMask    = pVisual[i].redMask;
                modes->greenMask  = pVisual[i].greenMask;
                modes->blueMask   = pVisual[i].blueMask;
                modes->alphaMask  = modes->alphaMask;
                modes->rgbBits = (is_rgb)
                    ? (modes->redBits + modes->greenBits +
                       modes->blueBits + modes->alphaBits)
                    : rootDepth;
            }

            /* Save the device-dependent private for this visual */
            glXVisualPriv[j] = pNewVisualPriv[k];

            j++;
            modes = modes->next;
        }
    }

    assert(j <= numNewVisuals);

    /* Save the GLX visuals in the screen structure */
    glWinScreens[screenInfo.numScreens-1].num_vis = numNewVisuals;
    glWinScreens[screenInfo.numScreens-1].priv = glXVisualPriv;

    /* Set up depth's VisualIDs */
    for (i = 0; i < ndepth; i++) {
        int numVids = 0;
        VisualID *pVids = NULL;
        int k, n = 0;

        /* Count the new number of VisualIDs at this depth */
        for (j = 0; j < pdepth[i].numVids; j++)
            for (k = 0; k < numNewVisuals; k++)
                if (pdepth[i].vids[j] == orig_vid[k])
                    numVids++;

        /* Allocate a new list of VisualIDs for this depth */
        pVids = (VisualID *)malloc(numVids * sizeof(VisualID));

        /* Initialize the new list of VisualIDs for this depth */
        for (j = 0; j < pdepth[i].numVids; j++)
            for (k = 0; k < numNewVisuals; k++)
                if (pdepth[i].vids[j] == orig_vid[k])
                    pVids[n++] = pVisualNew[k].vid;

        /* Update this depth's list of VisualIDs */
        free(pdepth[i].vids);
        pdepth[i].vids = pVids;
        pdepth[i].numVids = numVids;
    }

    /* Update the X server's visuals */
    *nvisualp = numNewVisuals;
    *visualp = pVisualNew;

    /* Free the old list of the X server's visuals */
    free(pVisual);

    /* Clean up temporary allocations */
    free(orig_vid);
    free(pNewVisualPriv);
    free(pNewVisualConfigs);

    /* Free the private list created by DDX HW driver */
    if (visualPrivates)
        free(visualPrivates);
    visualPrivates = NULL;

    return TRUE;
}


static void fixup_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    glWinScreenRec *pScr = &glWinScreens[screen];
    __GLcontextModes *modes;
    int j;

    GLWIN_DEBUG_MSG("fixup_visuals\n");

    for (modes = pScr->modes; modes != NULL; modes = modes->next ) {
        const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
        const int nplanes = (modes->rgbBits - modes->alphaBits);
        const VisualPtr pVis = pScreen->visuals;

        /* Find a visual that matches the GLX visual's class and size */
        for (j = 0; j < pScreen->numVisuals; j++) {
            if (pVis[j].class == vis_class &&
                pVis[j].nplanes == nplanes) {

                /* Fixup the masks */
                modes->redMask   = pVis[j].redMask;
                modes->greenMask = pVis[j].greenMask;
                modes->blueMask  = pVis[j].blueMask;

                /* Recalc the sizes */
                modes->redBits   = count_bits(modes->redMask);
                modes->greenBits = count_bits(modes->greenMask);
                modes->blueBits  = count_bits(modes->blueMask);
            }
        }
    }
}

static void init_screen_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    __GLcontextModes *modes;
    int *used;
    int i, j;

    GLWIN_DEBUG_MSG("init_screen_visuals\n");

    used = (int *)malloc(pScreen->numVisuals * sizeof(int));
    memset(used, 0, pScreen->numVisuals * sizeof(int));

    i = 0;
    for ( modes = glWinScreens[screen].modes
          ; modes != NULL
          ; modes = modes->next) {
        const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
        const int nplanes = (modes->rgbBits - modes->alphaBits);
        const VisualPtr pVis = pScreen->visuals;

        for (j = 0; j < pScreen->numVisuals; j++) {

            if (pVis[j].class     == vis_class &&
                pVis[j].nplanes   == nplanes &&
                pVis[j].redMask   == modes->redMask &&
                pVis[j].greenMask == modes->greenMask &&
                pVis[j].blueMask  == modes->blueMask &&
                !used[j]) {

                /* Set the VisualID */
                modes->visualID = pVis[j].vid;

                /* Mark this visual used */
                used[j] = 1;
                break;
            }
        }

        if ( j == pScreen->numVisuals ) {
            ErrorF("No matching visual for __GLcontextMode with "
                   "visual class = %d (%d), nplanes = %u\n",
                   vis_class, 
                   modes->visualType,
                   (modes->rgbBits - modes->alphaBits) );
        }
        else if ( modes->visualID == -1 ) {
            FatalError( "Matching visual found, but visualID still -1!\n" );
        }

        i++;
        
    }

    free(used);
}

#if 0
static __GLXscreen *
glWinScreenProbe(ScreenPtr pScreen)
{
    glWinScreenRec *screenPriv;

    GLWIN_DEBUG_MSG("glWinScreenProbe\n");

    /*
     * Set up the current screen's visuals.
     */
    __glDDXScreenInfo.modes = glWinScreens[screen].modes;
    __glDDXScreenInfo.pVisualPriv = glWinScreens[screen].priv;
    __glDDXScreenInfo.numVisuals = __glDDXScreenInfo.numUsableVisuals = glWinScreens[screen].num_vis;

    /*
     * Set the current screen's createContext routine.  This could be
     * wrapped by a DDX GLX context creation routine.
     */
    __glDDXScreenInfo.createContext = glWinCreateContext;

    /*
     * The ordering of the rgb compenents might have been changed by the
     * driver after mi initialized them.
     */
    fixup_visuals(screen);

    /*
     * Find the GLX visuals that are supported by this screen and create
     * XMesa's visuals.
     */
    init_screen_visuals(screen);

    /* Wrap RealizeWindow and UnrealizeWindow on this screen */
    screenPriv = &glWinScreens[screen];
    screenPriv->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = glWinRealizeWindow;
    screenPriv->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = glWinUnrealizeWindow;
    screenPriv->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = glWinCopyWindow;

    return TRUE;
}
#endif

static GLboolean glWinSwapBuffers(__GLXdrawablePrivate *glxPriv)
{
  /* swap buffers on only *one* of the contexts
   * (e.g. the last one for drawing)
   */
    if (!glxPriv->drawGlxc) return GL_TRUE; /* Colin: do same as gc == NULL and so prevent a bad dereference */

#if 0
    __GLcontext *gc = (__GLcontext *)glxPriv->drawGlxc->gc;
    HDC dc;
    BOOL ret;

    GLWIN_TRACE_MSG("glWinSwapBuffers (ctx %p)\n", (gc!=NULL?gc->ctx:NULL));

    if (gc != NULL && gc->ctx != NULL)
    {
        dc = glWinMakeDC(gc);
        if (dc == NULL)
            return GL_FALSE;

        ret = SwapBuffers(dc);
        if (!ret)
            ErrorF("SwapBuffers failed: %s\n", glWinErrorMessage());
        
        ReleaseDC(gc->winInfo.hwnd, dc);
        if (!ret)
            return GL_FALSE;
    }
#endif

    return GL_TRUE;
}

static void glWinDestroyDrawablePrivate(__GLdrawablePrivate *glPriv)
{
    GLWIN_DEBUG_MSG("glWinDestroyDrawablePrivate\n");

    /* It doesn't work to call DRIDestroySurface here, the drawable's
       already gone.. But dri.c notices the window destruction and
       frees the surface itself. */

    free(glPriv->private);
    glPriv->private = NULL;
}


#if 0
static void glWinCreateBuffer(__GLXdrawablePrivate *glxPriv)
{
    GLWinDrawableRec *winPriv = malloc(sizeof(GLWinDrawableRec));
    __GLdrawablePrivate *glPriv = &glxPriv->glPriv;

    /*winPriv->sid = 0; */
    winPriv->pDraw = NULL;

    GLWIN_DEBUG_MSG("glWinCreateBuffer\n");

    /* replace swapBuffers (original is never called) */
    glxPriv->swapBuffers = glWinSwapBuffers;

    /* stash private data */
    glPriv->private = winPriv;
    glPriv->freePrivate = glWinDestroyDrawablePrivate;
}
#endif

#if 0
static void glWinResetExtension(void)
{
    GLWIN_DEBUG_MSG("glWinResetExtension\n");
}
#endif

/* based on code in apples/indirect.c which is based on i830_dri.c */
static void
glWinInitVisualConfigs(void)
{
    int                 lclNumConfigs     = 0;
    __GLXvisualConfig  *lclVisualConfigs  = NULL;
    void              **lclVisualPrivates = NULL;

    int stereo, depth, aux, buffers, stencil, accum;
    int i = 0;

    GLWIN_DEBUG_MSG("glWinInitVisualConfigs\n");
        
    /* count num configs:
        2 stereo (on, off) (optional)
        2 Z buffer (0, 24 bit)
        2 AUX buffer (0, 2)
        2 buffers (single, double)
        2 stencil (0, 8 bit)
        2 accum (0, 64 bit)
        = 64 configs with stereo, or 32 without */

    if (glWinDebugSettings.enableStereo) lclNumConfigs = 2 * 2 * 2 * 2 * 2 * 2; /* 64 */
    else                                 lclNumConfigs = 2 * 2 * 2 * 2 * 2; /* 32 */

    /* alloc */
    lclVisualConfigs = xcalloc(sizeof(__GLXvisualConfig), lclNumConfigs);
    lclVisualPrivates = xcalloc(sizeof(void *), lclNumConfigs);

    /* fill in configs */
    if (NULL != lclVisualConfigs) {
        i = 0; /* current buffer */
        for (stereo = 0; stereo < (glWinDebugSettings.enableStereo ? 2 : 1); stereo++) {
	  for (depth = 0; depth < 2; depth++) {
            for (aux = 0; aux < 2; aux++) {
	      for (buffers = 0; buffers < 2; buffers++) {
		for (stencil = 0; stencil < 2; stencil++) {
		  for (accum = 0; accum < 2; accum++) {
		    lclVisualConfigs[i].vid = -1;
		    lclVisualConfigs[i].class = -1;
		    lclVisualConfigs[i].rgba = TRUE;
		    lclVisualConfigs[i].redSize = -1;
		    lclVisualConfigs[i].greenSize = -1;
		    lclVisualConfigs[i].blueSize = -1;
		    lclVisualConfigs[i].redMask = -1;
		    lclVisualConfigs[i].greenMask = -1;
		    lclVisualConfigs[i].blueMask = -1;
		    lclVisualConfigs[i].alphaMask = 0;
		    if (accum) {
		      lclVisualConfigs[i].accumRedSize = 16;
		      lclVisualConfigs[i].accumGreenSize = 16;
		      lclVisualConfigs[i].accumBlueSize = 16;
		      lclVisualConfigs[i].accumAlphaSize = 16;
		    } else {
		      lclVisualConfigs[i].accumRedSize = 0;
		      lclVisualConfigs[i].accumGreenSize = 0;
		      lclVisualConfigs[i].accumBlueSize = 0;
		      lclVisualConfigs[i].accumAlphaSize = 0;
		    }

		    lclVisualConfigs[i].bufferSize = -1;

		    lclVisualConfigs[i].depthSize = depth? 24 : 0;
		    lclVisualConfigs[i].stencilSize = stencil ? 8 : 0;
		    lclVisualConfigs[i].auxBuffers = aux ? 2 : 0;
		    lclVisualConfigs[i].level = 0;
		    lclVisualConfigs[i].visualRating = GLX_NONE_EXT;
		    lclVisualConfigs[i].transparentPixel = 0;
		    lclVisualConfigs[i].transparentRed = 0;
		    lclVisualConfigs[i].transparentGreen = 0;
		    lclVisualConfigs[i].transparentBlue = 0;
		    lclVisualConfigs[i].transparentAlpha = 0;
		    lclVisualConfigs[i].transparentIndex = 0;
		    i++;
		  }
		}
	      }
            }
	  }
	}
    }
    if (i != lclNumConfigs)
        GLWIN_DEBUG_MSG("glWinInitVisualConfigs failed to alloc visual configs");
}

#if 0
/* Copied from Mesa */
static void glWinSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
                                   void **privates)
{
    GLWIN_DEBUG_MSG("glWinSetVisualConfigs\n");

    numConfigs = nconfigs;
    visualConfigs = configs;
    visualPrivates = privates;
}
#endif

#if 0
/* Copied from Mesa */
static Bool glWinInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
                              int *nvisualp, int *ndepthp,
                              int *rootDepthp, VisualID *defaultVisp,
                              unsigned long sizes, int bitsPerRGB)
{
    glWinInitDebugSettings();

    GLWIN_DEBUG_MSG("glWinInitVisuals\n");

    if (0 == numConfigs) /* if no configs */
        glWinInitVisualConfigs(); /* ensure the visual configs are setup */

    /*
     * Setup the visuals supported by this particular screen.
     */
    return init_visuals(nvisualp, visualp, defaultVisp,
                        *ndepthp, *depthp, *rootDepthp);
}
#endif

/* ---------------------------------------------------------------------- */

static __GLXcontext *
glWinCreateContext(__GLXscreen *screen,
                   __GLXconfig *modes,
                   __GLXcontext *shareContext)
{
    __GLXcontext *result;

    GLWIN_DEBUG_MSG("glWinCreateContext\n");

    result = (__GLXcontext *)calloc(1, sizeof(__GLXcontext));

    if (!result)
        return NULL;

/*     if (makeFormat(mode, &result->pfd)) */
/*     { */
/*         ErrorF("makeFormat failed\n"); */
/*         free(result); */
/*         return NULL; */
/*     } */

/*     if (glWinDebugSettings.dumpPFD) */
/*         pfdOut(&result->pfd); */

    GLWIN_DEBUG_MSG("glWinCreateContext done\n");

    return result;
}

static __GLXconfig *
glWinCreateConfigs(int *numConfigsPtr, int screenNumber)
{
  __GLXconfig *c, *result;
  int numConfigs = 0;
  int i;

  GLWIN_DEBUG_MSG("glWinCreateConfigs\n");

  /* count num configs:
     2 stereo (on, off) (optional)
     2 Z buffer (0, 24 bit)
     2 AUX buffer (0, 2)
     2 buffers (single, double)
     2 stencil (0, 8 bit)
     2 accum (0, 64 bit)
     = 64 configs with stereo, or 32 without */

  if (glWinDebugSettings.enableStereo)
    numConfigs = 2 * 2 * 2 * 2 * 2 * 2; /* 64 */
  else
    numConfigs = 2 * 2 * 2 * 2 * 2; /* 32 */

  *numConfigsPtr = numConfigs;

  /* alloc */
  c = xalloc(sizeof(*c) * numConfigs);

  if(NULL == c)
    return NULL;

  result = c;

  /* fill in configs */
  memset(result, 0, sizeof(*c) * numConfigs);

  i = 0;

  int stereo, aux, buffers, stencil, accum, depth;
  // msample, color, ???

  for (stereo = 0; stereo < (glWinDebugSettings.enableStereo ? 2 : 1); stereo++)
    {
      for (depth = 0; depth < 2; ++depth)
        {
          for (aux = 0; aux < 2; ++aux)
            {
              for (buffers = 0; buffers < 2; ++buffers)
                {
                  for (stencil = 0; stencil < 2; ++stencil)
                    {
                      for (accum = 0; accum < 2; ++accum)
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

                          c->doubleBufferMode = buffers ? GL_TRUE : GL_FALSE;
                          c->stereoMode = stereo ? GL_TRUE : GL_FALSE;

                          c->redBits = (depth+1)*8;
                          c->greenBits = (depth+1)*8;
                          c->blueBits = (depth+1)*8;
                          c->alphaBits = 0;

                          c->redMask = -1;
                          c->greenMask = -1;
                          c->blueMask = -1;
                          c->alphaMask = -1;

                          c->rgbBits = c->redBits + c->greenBits + c->blueBits + c->alphaBits;
                          c->indexBits = 0;

                          c->accumRedBits = 0;
                          c->accumGreenBits = 0;
                          c->accumBlueBits = 0;
                          c->accumAlphaBits = 0;

                          if (accum)
                            {
                              c->accumRedBits = 16;
                              c->accumGreenBits = 16;
                              c->accumBlueBits = 16;
                              c->accumAlphaBits = 16;
                            }

                          c->depthBits = depth ? 24 : 0;
                          c->stencilBits = 0;

                          if (stencil)
                            {
                              c->stencilBits = 8;
                            }

                          c->numAuxBuffers = aux ? 2 : 0;

                          c->level = 0;
                          c->pixmapMode = 0;
                          c->visualID = -1;
                          c->visualType = GLX_TRUE_COLOR;
                          c->visualRating = GLX_NONE_EXT;

                          c->transparentPixel = GLX_NONE;
                          c->transparentRed = GLX_NONE;
                          c->transparentGreen = GLX_NONE;
                          c->transparentAlpha = GLX_NONE;
                          c->transparentIndex = GLX_NONE;

                          c->sampleBuffers = 0;
                          c->samples = 0;

                          c->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
                          c->renderType = GLX_RGBA_BIT;
                          c->xRenderable = GL_TRUE;
                          c->fbconfigID = -1;

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

                          ++i;
                        }
                    }
                }
            }
        }
    }

  if (i != numConfigs)
    FatalError("The number of __GLXconfig generated does not match the initial calculation!\n");

  return result;
}


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

    screen->base.destroy = NULL; //__glXAquaScreenDestroy;
    screen->base.createContext = glWinCreateContext;
    screen->base.createDrawable = NULL; // __glXAquaScreenCreateDrawable;
    screen->base.swapInterval = NULL;
    screen->base.hyperpipeFuncs = NULL;
    screen->base.swapBarrierFuncs = NULL;
    screen->base.pScreen = pScreen;
    screen->base.fbconfigs = glWinCreateConfigs(&screen->base.numFBConfigs,
                                                pScreen->myNum);
    // Note that screen->base.numFBConfigs is also initialized by that

    /* This is set by __glXScreenInit: */
    screen->base.visuals = NULL;
    /* This is to be initialized prior to the call to __glXScreenInit: */
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
    /*We may be able to add more GLXextensions at a later time. */

    return &screen->base;
}

__GLXprovider __glXWGLProvider = {
    glWinScreenProbe,
    "Win32 native WGL OpenGL32",
    NULL
};

void
glWinPushNativeProvider(void)
{
  GlxPushProvider(&__glXWGLProvider);
}

/* ---------------------------------------------------------------------- */
