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

/*
  TODO:
  - hook up remaining unimplemented extensions
  - research what guarantees glXWaitX, glXWaitGL are supposed to offer, and implement then
    using GdiFlush and/or glFinish
  - pbuffer clobbering: we don't get async notification, but can we arrange to emit the
    event when we notice it's been clobbered? at the very least, check if it's been clobbered
    before using it?
  - SetPixelFormat on the screen DC is rude and inconsiderate to others: create an invisible
    window instead
  - are the __GLXConfig * we get handed back ones we are made (so we can extend the structure
    with privates?) Or are they created inside the GLX core as well?
*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "glwindows.h"
#include <glx/glxserver.h>
#include <glx/glxutil.h>
#include <glx/extension_string.h>
#include <GL/internal/glcore.h>
#include <GL/glxtokens.h>

#include <winpriv.h>
#include <wgl_ext_api.h>

/* ---------------------------------------------------------------------- */
/*
 * Various debug helpers
 */

#define GLWIN_DEBUG_HWND(hwnd)  \
    if (glxWinDebugSettings.dumpHWND) { \
        char buffer[1024]; \
        if (GetWindowText(hwnd, buffer, sizeof(buffer))==0) *buffer=0; \
        GLWIN_DEBUG_MSG("Got HWND %p for window '%s'", hwnd, buffer); \
    }

glxWinDebugSettingsRec glxWinDebugSettings = { 0, 0, 0, 0, 0, 0};

static void glxWinInitDebugSettings(void)
{
    char *envptr;

    envptr = getenv("GLWIN_ENABLE_DEBUG");
    if (envptr != NULL)
        glxWinDebugSettings.enableDebug = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_ENABLE_TRACE");
    if (envptr != NULL)
        glxWinDebugSettings.enableTrace = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_PFD");
    if (envptr != NULL)
        glxWinDebugSettings.dumpPFD = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_HWND");
    if (envptr != NULL)
        glxWinDebugSettings.dumpHWND = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DUMP_DC");
    if (envptr != NULL)
        glxWinDebugSettings.dumpDC = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_ENABLE_GLCALL_TRACE");
    if (envptr != NULL)
        glxWinDebugSettings.enableGLcallTrace = (atoi(envptr) == 1);

    envptr = getenv("GLWIN_DEBUG_ALL");
    if (envptr != NULL)
      {
        glxWinDebugSettings.enableDebug = 1;
        glxWinDebugSettings.enableTrace = 1;
        glxWinDebugSettings.dumpPFD = 1;
        glxWinDebugSettings.dumpHWND = 1;
        glxWinDebugSettings.dumpDC = 1;
        glxWinDebugSettings.enableGLcallTrace = 1;
      }
}

static
const char *glxWinErrorMessage(void)
{
  static char errorbuffer[1024];

  if (!FormatMessage(
                     FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,
                     GetLastError(),
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR) &errorbuffer,
                     sizeof(errorbuffer),
                     NULL ))
    {
      snprintf(errorbuffer, sizeof(errorbuffer), "Unknown error in FormatMessage: %08x!", (unsigned)GetLastError());
    }

  if (errorbuffer[strlen(errorbuffer)-1] == '\n')
    errorbuffer[strlen(errorbuffer)-1] = 0;

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

static const char *
visual_class_name(int cls)
{
  switch (cls) {
  case GLX_STATIC_COLOR:
    return "StaticColor";
  case GLX_PSEUDO_COLOR:
    return "PseudoColor";
  case GLX_STATIC_GRAY:
    return "StaticGray";
  case GLX_GRAY_SCALE:
    return "GrayScale";
  case GLX_TRUE_COLOR:
     return "TrueColor";
  case GLX_DIRECT_COLOR:
     return "DirectColor";
  default:
    return "-none-";
  }
}

static const char *
swap_method_name(int mthd)
{
  switch (mthd)
    {
    case GLX_SWAP_EXCHANGE_OML:
      return "xchg";
    case GLX_SWAP_COPY_OML:
      return "copy";
    case GLX_SWAP_UNDEFINED_OML:
      return "    ";
    default:
      return "????";
    }
}

static void
fbConfigsDump(unsigned int n, __GLXconfig *configs)
{
  __GLXconfig *c = configs;

  printf("%d fbConfigs\n", n);
  printf("pxf vis  fb                      render         Ste                     aux    accum        MS    drawable             Group/\n");
  printf("idx  ID  ID VisualType Depth Lvl RGB CI DB Swap reo  R  G  B  A   Z  S  buf AR AG AB AA  bufs num  W P Pb  Float Trans Caveat\n");
  printf("-----------------------------------------------------------------------------------------------------------------------------\n");

  while (c != NULL)
    {
      unsigned int i = c - configs;

      printf("%3d  %2x  %2x ", i+1, c->visualID, c->fbconfigID);
      printf("%-11s", visual_class_name(c->visualType));
      printf(" %3d %3d   %s   %s  %s %s  %s  ",
             c->rgbBits ? c->rgbBits : c->indexBits,
             c->level,
	     (c->renderType & GLX_RGBA_BIT) ? "y" : ".",
	     (c->renderType & GLX_COLOR_INDEX_BIT) ? "y" : ".",
	     c->doubleBufferMode ? "y" : ".",
             swap_method_name(c->swapMethod),
	     c->stereoMode ? "y" : ".");
      printf("%2d %2d %2d %2d  ", c->redBits, c->greenBits, c->blueBits, c->alphaBits);
      printf("%2d %2d  ", c->depthBits, c->stencilBits);
      printf("%2d  ", c->numAuxBuffers);
      printf("%2d %2d %2d %2d", c->accumRedBits, c->accumGreenBits, c->accumBlueBits, c->accumAlphaBits);
      printf("   %2d   %2d", c->sampleBuffers, c->samples);
      printf("  %s %s %s ", (c->drawableType & GLX_WINDOW_BIT) ? "y" : ".",
             (c->drawableType & GLX_PIXMAP_BIT) ? "y" : ".",
             (c->drawableType & GLX_PBUFFER_BIT) ? "y" : ".");
      printf("    %s   ",".");
      printf("  %s   ", (c->transparentPixel != GLX_NONE_EXT) ? "y" : ".");
      printf("  %d %s", c->visualSelectGroup, (c->visualRating == GLX_SLOW_VISUAL_EXT) ? "*" : " ");
      printf("\n");

      c = c->next;
    }
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
  __GLXWinContext *shareContext;     /* Context with which we will share display lists and textures */
  HWND hwnd;                         /* For detecting when HWND has changed */
  __GLXconfig *config;               /* Selected fbConfig */
};

struct __GLXWinDrawable
{
  __GLXdrawable base;
  __GLXWinContext *drawContext;
  __GLXWinContext *readContext;

  /* If this drawable is GLX_DRAWABLE_PBUFFER */
  HPBUFFERARB hPbuffer;

  /* If this drawable is GLX_DRAWABLE_PIXMAP */
  HDC dibDC;
  HBITMAP hDIB;
  HBITMAP hOldDIB; /* original DIB for DC */
  void *pOldBits; /* original pBits for this drawable's pixmap */
};

typedef struct {
    __GLXscreen base;

    /* Supported GLX extensions */
    unsigned char glx_enable_bits[__GLX_EXT_BYTES];

    Bool has_WGL_ARB_pixel_format;

    /* wrapped screen functions */
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    CopyWindowProcPtr CopyWindow;
} glxWinScreen;

/* ---------------------------------------------------------------------- */
/*
 * Forward declarations
 */

static __GLXscreen *glxWinScreenProbe(ScreenPtr pScreen);
static __GLXcontext *glxWinCreateContext(__GLXscreen *screen,
                                        __GLXconfig *modes,
                                        __GLXcontext *baseShareContext);
static __GLXdrawable *glxWinCreateDrawable(__GLXscreen *screen,
                                          DrawablePtr pDraw,
                                          int type,
                                          XID drawId,
                                          __GLXconfig *conf);

static Bool glxWinRealizeWindow(WindowPtr pWin);
static Bool glxWinUnrealizeWindow(WindowPtr pWin);
static void glxWinCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc);

static HDC glxWinMakeDC(__GLXWinContext *gc, __GLXWinDrawable *draw, HDC *hdc, HWND *hwnd);
static void glxWinReleaseDC(HWND hwnd, HDC hdc, __GLXWinDrawable *draw);

static __GLXconfig *glxWinCreateConfigs(HDC dc, int *numConfigsPtr, int screenNumber);
static __GLXconfig *glxWinCreateConfigsExt(HDC dc, int *numConfigsPtr, int screenNumber);
static int fbConfigToPixelFormat(__GLXconfig *mode, PIXELFORMATDESCRIPTOR *pfdret, int drawableTypeOverride);
static int fbConfigToPixelFormatIndex(HDC hdc, __GLXconfig *mode, int drawableTypeOverride);

/* ---------------------------------------------------------------------- */
/*
 * The GLX provider
 */

__GLXprovider __glXWGLProvider = {
    glxWinScreenProbe,
    "Win32 native WGL",
    NULL
};

void
glxWinPushNativeProvider(void)
{
  GlxPushProvider(&__glXWGLProvider);
}

/* ---------------------------------------------------------------------- */
/*
 * Screen functions
 */

static void
glxWinScreenDestroy(__GLXscreen *screen)
{
    GLWIN_DEBUG_MSG("glxWinScreenDestroy(%p)", screen);
    __glXScreenDestroy(screen);
    xfree(screen);
}

static int
glxWinScreenSwapInterval(__GLXdrawable *drawable, int interval)
{
  BOOL ret = wglSwapIntervalEXTWrapper(interval);
  if (!ret)
    {
      ErrorF("wglSwapIntervalEXT interval %d failed:%s\n", interval, glxWinErrorMessage());
    }
  return ret;
}

/* This is called by GlxExtensionInit() asking the GLX provider if it can handle the screen... */
static __GLXscreen *
glxWinScreenProbe(ScreenPtr pScreen)
{
    glxWinScreen *screen;
    const char *gl_extensions;
    const char *wgl_extensions;
    HDC hdc;
    HGLRC hglrc;

    GLWIN_DEBUG_MSG("glxWinScreenProbe");

    glxWinInitDebugSettings();

    if (pScreen == NULL)
	return NULL;

    if (!winCheckScreenIsSupported(pScreen))
      {
        LogMessage(X_ERROR,"AIGLX: No native OpenGL in modes with a root window\n");
        return NULL;
      }

    screen = xcalloc(1, sizeof(glxWinScreen));

    if (NULL == screen)
	return NULL;

    /* Wrap RealizeWindow, UnrealizeWindow and CopyWindow on this screen */
    screen->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = glxWinRealizeWindow;
    screen->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = glxWinUnrealizeWindow;
    screen->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = glxWinCopyWindow;

    /* Dump out some useful information about the native renderer */
    // just use the screen DC
    hdc = GetDC(NULL);
    // we must set a pixel format before we can create a context, just use the first one...
    // MSDN says SetPixelFormat()'s PIXELFORMATDESCRIPTOR pointer argument has no effect
    // except on metafiles, so I choose to interpret that as it's ok to supply NULL  if
    // the DC is not for a metafile...
    SetPixelFormat(hdc, 1, NULL);
    hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    // initialize wgl extension proc pointers (don't call them before here...)
    // (but we need to have a current context for them to be resolvable)
    wglResolveExtensionProcs();

    ErrorF("GL_VERSION:    %s\n", glGetStringWrapperNonstatic(GL_VERSION));
    ErrorF("GL_VENDOR:     %s\n", glGetStringWrapperNonstatic(GL_VENDOR));
    ErrorF("GL_RENDERER:   %s\n", glGetStringWrapperNonstatic(GL_RENDERER));
    gl_extensions = glGetStringWrapperNonstatic(GL_EXTENSIONS);
    ErrorF("GL_EXTENSIONS: %s\n", gl_extensions);
    wgl_extensions = wglGetExtensionsStringARBWrapper(hdc);
    ErrorF("WGL_EXTENSIONS:%s\n", wgl_extensions);

    // Can you see the problem here?  The extensions string is DC specific
    // Different DCs for windows on a multimonitor system driven by multiple cards
    // might have completely different capabilities.  Of course, good luck getting
    // those screens to be accelerated in XP and earlier...

    {
      // testing facility to not use any WGL extensions
      char *envptr = getenv("GLWIN_NO_WGL_EXTENSIONS");
      if ((envptr != NULL) && (atoi(envptr) != 0))
        {
          ErrorF("GLWIN_NO_WGL_EXTENSIONS is set, ignoring WGL_EXTENSIONS\n");
          wgl_extensions = "";
        }
    }

    {
      Bool glx_sgix_pbuffer = FALSE;
      Bool glx_sgi_make_current_read = FALSE;
      Bool glx_arb_multisample = FALSE;

      //
      // Based on the WGL extensions available, enable various GLX extensions
      // XXX: make this table-driven ?
      //
      memset(screen->glx_enable_bits, 0, __GLX_EXT_BYTES);

      __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_visual_info");
      __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_visual_rating");
      __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_import_context");
      __glXEnableExtension(screen->glx_enable_bits, "GLX_OML_swap_method");
      __glXEnableExtension(screen->glx_enable_bits, "GLX_SGIX_fbconfig");

      if (strstr(wgl_extensions, "WGL_ARB_make_current_read"))
        {
          __glXEnableExtension(screen->glx_enable_bits, "GLX_SGI_make_current_read");
          LogMessage(X_INFO, "AIGLX: enabled GLX_SGI_make_current_read\n");
          glx_sgi_make_current_read = TRUE;
        }

      if (strstr(gl_extensions, "GL_WIN_swap_hint"))
        {
          __glXEnableExtension(screen->glx_enable_bits, "GLX_MESA_copy_sub_buffer");
          LogMessage(X_INFO, "AIGLX: enabled GLX_MESA_copy_sub_buffer\n");
        }

      if (strstr(wgl_extensions, "WGL_EXT_swap_control"))
        {
          __glXEnableExtension(screen->glx_enable_bits, "GLX_SGI_swap_control");
          __glXEnableExtension(screen->glx_enable_bits, "GLX_MESA_swap_control");
          LogMessage(X_INFO, "AIGLX: enabled GLX_SGI_swap_control and GLX_MESA_swap_control\n");
        }

/*       // Hmm?  screen->texOffset */
/*       if (strstr(wgl_extensions, "WGL_ARB_render_texture")) */
/*         { */
/*           __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_texture_from_pixmap"); */
/*           LogMessage(X_INFO, "AIGLX: GLX_EXT_texture_from_pixmap backed by buffer objects\n"); */
/*         } */

      if (strstr(wgl_extensions, "WGL_ARB_pbuffer"))
        {
          __glXEnableExtension(screen->glx_enable_bits, "GLX_SGIX_pbuffer");
          LogMessage(X_INFO, "AIGLX: enabled GLX_SGIX_pbuffer\n");
          glx_sgix_pbuffer = TRUE;
        }

/*       if (strstr(wgl_extensions, "WGL_ARB_multisample")) */
/*         { */
/*           __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_multisample"); */
/*           LogMessage(X_INFO, "AIGLX: enabled GLX_ARB_multisample\n"); */
/*           glx_arb_multisample = TRUE; */
/*           // also "GLX_SGIS_multisample "? */
/*         } */


      screen->base.destroy = glxWinScreenDestroy;
      screen->base.createContext = glxWinCreateContext;
      screen->base.createDrawable = glxWinCreateDrawable;
      screen->base.swapInterval = glxWinScreenSwapInterval;
      screen->base.hyperpipeFuncs = NULL;
      screen->base.swapBarrierFuncs = NULL;
      screen->base.pScreen = pScreen;

      if (strstr(wgl_extensions, "WGL_ARB_pixel_format"))
        {
          screen->base.fbconfigs = glxWinCreateConfigsExt(hdc, &screen->base.numFBConfigs, pScreen->myNum);
          screen->has_WGL_ARB_pixel_format = TRUE;
        }
      else
        {
          screen->base.fbconfigs = glxWinCreateConfigs(hdc, &screen->base.numFBConfigs, pScreen->myNum);
          screen->has_WGL_ARB_pixel_format = FALSE;
        }
      // Note that screen->base.numFBConfigs is also initialized by the above

      /* These will be set by __glXScreenInit */
      screen->base.visuals = NULL;
      screen->base.numVisuals = 0;

      __glXScreenInit(&screen->base, pScreen);

      // dump out fbConfigs now visualIDs have been assigned
      fbConfigsDump(screen->base.numFBConfigs, screen->base.fbconfigs);

      // Override the GL extensions string set by __glXScreenInit()
      screen->base.GLextensions = xstrdup(gl_extensions);

      // Generate the GLX extensions string (overrides that set by __glXScreenInit())
      {
        unsigned int buffer_size = __glXGetExtensionString(screen->glx_enable_bits, NULL);
        if (buffer_size > 0)
          {
            if (screen->base.GLXextensions != NULL)
              {
                xfree(screen->base.GLXextensions);
              }

            screen->base.GLXextensions = xnfalloc(buffer_size);
            __glXGetExtensionString(screen->glx_enable_bits, screen->base.GLXextensions);
          }
      }

      //
      // Override the GLX version (__glXScreenInit() sets it to "1.2")
      // if we have all the needed extensionsto operate as a higher version
      //
      // SGIX_fbconfig && SGIX_pbuffer && SGI_make_current_read -> 1.3
      // ARB_multisample -> 1.4
      //
      if (glx_sgix_pbuffer && glx_sgi_make_current_read)
        {
          xfree(screen->base.GLXversion);

          if (glx_arb_multisample)
            {
              screen->base.GLXversion = xstrdup("1.4");
              /*
                XXX: this just controls the version string returned to glXQueryServerString()
                there is currently no way to control the version number the server returns to
                glXQueryVersion()...
              */
            }
          else
            {
              screen->base.GLXversion = xstrdup("1.3");
            }
          LogMessage(X_INFO, "AIGLX: Set GLX version to %s\n", screen->base.GLXversion);
        }
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(NULL, hdc);


    return &screen->base;
}

/* ---------------------------------------------------------------------- */
/*
 * Window functions
 */

static Bool
glxWinRealizeWindow(WindowPtr pWin)
{
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glxWinScreen *screenPriv = (glxWinScreen *) glxGetScreen(pScreen);

    GLWIN_DEBUG_MSG("glxWinRealizeWindow");

    /* Allow the window to be created (RootlessRealizeWindow is inside our wrap) */
    pScreen->RealizeWindow = screenPriv->RealizeWindow;
    result = pScreen->RealizeWindow(pWin);
    pScreen->RealizeWindow = glxWinRealizeWindow;

    return result;
}


static void
glxWinCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    glxWinScreen *screenPriv = (glxWinScreen *) glxGetScreen(pScreen);

    GLWIN_TRACE_MSG("glxWinCopyWindow pWindow %p", pWindow);

    /*
       We used to discard any normal drawing requests if a GL drawing context
       was pointing at the window.... Not sure what that helps with...
    */

    GLWIN_DEBUG_MSG("glxWinCopyWindow - passing to hw layer");

    pScreen->CopyWindow = screenPriv->CopyWindow;
    pScreen->CopyWindow(pWindow, ptOldOrg, prgnSrc);
    pScreen->CopyWindow = glxWinCopyWindow;
}

static Bool
glxWinUnrealizeWindow(WindowPtr pWin)
{
    Bool result;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    glxWinScreen *screenPriv = (glxWinScreen *)glxGetScreen(pScreen);

    GLWIN_DEBUG_MSG("glxWinUnrealizeWindow");

    pScreen->UnrealizeWindow = screenPriv->UnrealizeWindow;
    result = pScreen->UnrealizeWindow(pWin);
    pScreen->UnrealizeWindow = glxWinUnrealizeWindow;

    return result;
}

/* ---------------------------------------------------------------------- */
/*
 * Drawable functions
 */

static GLboolean
glxWinDrawableSwapBuffers(__GLXdrawable *base)
{
    HDC dc;
    HWND hwnd;
    BOOL ret;
    __GLXWinDrawable *draw = (__GLXWinDrawable *)base;

    /* Swap buffers on the last active context for drawing on the drawable */
    if (draw->drawContext == NULL)
      {
        GLWIN_TRACE_MSG("glxWinSwapBuffers - no context for drawable");
        return GL_FALSE;
      }

    GLWIN_TRACE_MSG("glxWinSwapBuffers on drawable %p, last context %p (native ctx %p)", base, draw->drawContext, draw->drawContext->ctx);

    /*
       draw->drawContext->base.drawPriv will not be set if the context is not current anymore,
       but if it is, it should point to this drawable....
    */
    assert((draw->drawContext->base.drawPriv == NULL) || (draw->drawContext->base.drawPriv == base));

    dc = glxWinMakeDC(draw->drawContext, draw, &dc, &hwnd);
    if (dc == NULL)
      return GL_FALSE;

    ret = wglSwapLayerBuffers(dc, WGL_SWAP_MAIN_PLANE);

    glxWinReleaseDC(hwnd, dc, draw);

    if (!ret)
      {
        ErrorF("wglSwapBuffers failed: %s\n", glxWinErrorMessage());
        return GL_FALSE;
      }

    return GL_TRUE;
}

static void
glxWinDrawableCopySubBuffer(__GLXdrawable *drawable,
                            int x, int y, int w, int h)
{
  glAddSwapHintRectWINWrapperNonstatic(x, y, w, h);
  glxWinDrawableSwapBuffers(drawable);
}

static void
glxWinDrawableDestroy(__GLXdrawable *base)
{
  __GLXWinDrawable *glxPriv = (__GLXWinDrawable *)base;

  // XXX: really we need a list of all contexts pointing at this drawable, not just the last one...
  glxPriv->drawContext = NULL;
  glxPriv->readContext = NULL;

  if (glxPriv->hPbuffer)
    if (!wglDestroyPbufferARBWrapper(glxPriv->hPbuffer))
      {
        ErrorF("wglDestroyPbufferARB failed: %s\n", glxWinErrorMessage());
      }

  if (glxPriv->dibDC)
    {
      // restore the default DIB
      SelectObject(glxPriv->dibDC, glxPriv->hOldDIB);

      if (!DeleteDC(glxPriv->dibDC))
        {
          ErrorF("DeleteDC failed: %s\n", glxWinErrorMessage());
        }
    }

  if (glxPriv->hDIB)
    {
      if (!DeleteObject(glxPriv->hDIB))
        {
          ErrorF("DeleteObject failed: %s\n", glxWinErrorMessage());
        }

      ((PixmapPtr)glxPriv->base.pDraw)->devPrivate.ptr = glxPriv->pOldBits;
    }

  GLWIN_DEBUG_MSG("glxWinDestroyDrawable");
  xfree(glxPriv);
}

static __GLXdrawable *
glxWinCreateDrawable(__GLXscreen *screen,
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

  glxPriv->base.destroy       = glxWinDrawableDestroy;
  glxPriv->base.swapBuffers   = glxWinDrawableSwapBuffers;
  glxPriv->base.copySubBuffer = glxWinDrawableCopySubBuffer;
  // glxPriv->base.waitX  what are these for?
  // glxPriv->base.waitGL

  GLWIN_DEBUG_MSG("glxWinCreateDrawable %p", glxPriv);

  return &glxPriv->base;
}

/* ---------------------------------------------------------------------- */
/*
 * Texture functions
 */

// XXX: don't offer GLX_EXT_texture_from_pixmap if we can't implement these..

static
int glxWinBindTexImage(__GLXcontext  *baseContext,
                      int            buffer,
                      __GLXdrawable *pixmap)
{
  ErrorF("glxWinBindTexImage: not implemented\n");
  return FALSE;
}

static
int glxWinReleaseTexImage(__GLXcontext  *baseContext,
                         int            buffer,
                         __GLXdrawable *pixmap)
{
  ErrorF(" glxWinReleaseTexImage: not implemented\n");
  return FALSE;
}

/* ---------------------------------------------------------------------- */
/*
 * Lazy update context implementation
 *
 * WGL contexts are created for a specific HDC, so we cannot create the WGL
 * context in glxWinCreateContext(), we must defer creation until the context
 * is actually used on a specifc drawable which is connected to a native window.
 *
 * The WGL context may be used on other, compatible HDCs, so we don't need to
 * recreate it for every new native window
 *
 * XXX: I wonder why we can't create the WGL context on the screen HDC ?
 * Basically we assume all HDCs are comaptible at the moment: if they are not
 * we are in a muddle, there was some code in the old implementation to attempt
 * to transparently migrate a context to a new DC by copying state and sharing
 * lists with the old one...
 */

static void
glxWinSetPixelFormat(__GLXWinContext *gc, HDC hdc, int bppOverride, int drawableTypeOverride)
{
  __GLXscreen *screen = gc->base.pGlxScreen;
  glxWinScreen *winScreen = (glxWinScreen *)screen;

  if (!winScreen->has_WGL_ARB_pixel_format)
    {
      PIXELFORMATDESCRIPTOR pfd;
      int pixelFormat;

      /* convert fbConfig to PFD */
      if (fbConfigToPixelFormat(gc->config, &pfd, drawableTypeOverride))
        {
          ErrorF("glxWinSetPixelFormat: fbConfigToPixelFormat failed\n");
          return;
        }

      if (glxWinDebugSettings.dumpPFD)
        pfdOut(&pfd);

      /*
         When PFD_DRAW_TO_BITMAP is set, ChoosePixelFormat() always returns a format with
         the cColorBits we asked for, so we need to ensure it matches the bpp of the bitmap
      */
      if (bppOverride)
        {
          ErrorF("Forcing bpp from %d to %d\n", pfd.cColorBits, bppOverride);
          pfd.cColorBits = bppOverride;
        }

      /* We must select a pixelformat, but SetPixelFormat can only be called once for a window... */
      pixelFormat = ChoosePixelFormat(hdc, &pfd);
      if (pixelFormat == 0)
        {
          ErrorF("ChoosePixelFormat error: %s\n", glxWinErrorMessage());
          return;
        }

      GLWIN_DEBUG_MSG("ChoosePixelFormat: chose pixelFormatIndex %d", pixelFormat);

      if (!SetPixelFormat(hdc, pixelFormat, &pfd))
        {
          ErrorF("SetPixelFormat error: %s\n", glxWinErrorMessage());
          return;
        }
    }
  else
    {
      int pixelFormat = fbConfigToPixelFormatIndex(hdc, gc->config, drawableTypeOverride);
      if (pixelFormat == 0)
        {
          ErrorF("wglChoosePixelFormat error: %s\n", glxWinErrorMessage());
          return;
        }

      GLWIN_DEBUG_MSG("wglChoosePixelFormat: chose pixelFormatIndex %d", pixelFormat);

      if (!SetPixelFormat(hdc, pixelFormat, NULL))
        {
          ErrorF("SetPixelFormat error: %s\n", glxWinErrorMessage());
          return;
        }
    }
}

static HDC
glxWinMakeDC(__GLXWinContext *gc, __GLXWinDrawable *draw, HDC *hdc, HWND *hwnd)
{
  *hdc = NULL;
  *hwnd = NULL;

  if (draw == NULL)
    {
      GLWIN_TRACE_MSG("No drawable for context %p (native ctx %p)", gc, gc->ctx);
      return NULL;
    }

  switch (draw->base.type)
  {
    case GLX_DRAWABLE_WINDOW:
    {
      winWindowInfoRec winInfo;
      WindowPtr pWin;

      pWin = (WindowPtr) draw->base.pDraw;
      if (pWin == NULL)
        {
          GLWIN_TRACE_MSG("for drawable %p, no WindowPtr", pWin);
          return NULL;
        }

      winGetWindowInfo(pWin, &winInfo);
      *hwnd = winInfo.hwnd;

      if (winInfo.hwnd == NULL)
        {
          ErrorF("No HWND error: %s\n", glxWinErrorMessage());
          return NULL;
        }

      *hdc = GetDC(winInfo.hwnd);

      if (*hdc == NULL)
        ErrorF("GetDC error: %s\n", glxWinErrorMessage());

      /* Check if the hwnd has changed... */
      if (winInfo.hwnd != gc->hwnd)
        {
          if (glxWinDebugSettings.enableTrace)
            GLWIN_DEBUG_HWND(winInfo.hwnd);

          GLWIN_TRACE_MSG("for context %p (native ctx %p), hWnd changed from %p to %p", gc, gc->ctx, gc->hwnd, winInfo.hwnd);
          gc->hwnd = winInfo.hwnd;

          glxWinSetPixelFormat(gc, *hdc, 0, GLX_WINDOW_BIT);
        }
    }
    break;

  case GLX_DRAWABLE_PBUFFER:
    {
      *hdc = wglGetPbufferDCARBWrapper(draw->hPbuffer);

      if (*hdc == NULL)
        ErrorF("GetDC (pbuffer) error: %s\n", glxWinErrorMessage());
    }
    break;

  case GLX_DRAWABLE_PIXMAP:
    {
      *hdc = draw->dibDC;
    }
    break;

  default:
    {
      ErrorF("glxWinMakeDC: tried to makeDC for unhandled drawable type %d\n", draw->base.type);
    }
  }

  if (glxWinDebugSettings.dumpDC)
    GLWIN_DEBUG_MSG("Got HDC %p", *hdc);

  return *hdc;
}

static void
glxWinReleaseDC(HWND hwnd, HDC hdc,__GLXWinDrawable *draw)
{
  switch (draw->base.type)
  {
    case GLX_DRAWABLE_WINDOW:
    {
      ReleaseDC(hwnd, hdc);
    }
    break;

  case GLX_DRAWABLE_PBUFFER:
    {
      if (!wglReleasePbufferDCARBWrapper(draw->hPbuffer, hdc))
        {
          ErrorF("wglReleasePbufferDCARB error: %s\n", glxWinErrorMessage());
        }
    }
    break;

  case GLX_DRAWABLE_PIXMAP:
    {
      // don't release DC, the memory DC lives as long as the bitma

      // We must ensure that all GDI drawing into the bitmap has completed
      // in case we subsequently access the bits from it
      GdiFlush();
    }
    break;

  default:
    {
      ErrorF("glxWinReleaseDC: tried to releaseDC for unhandled drawable type %d\n", draw->base.type);
    }
  }
}

static void
glxWinDeferredCreateContext(__GLXWinContext *gc, __GLXWinDrawable *draw)
{
  HDC dc;
  HWND hwnd;
  GLWIN_DEBUG_MSG("glxWinDeferredCreateContext: attach context %p to drawable %p", gc, draw);

  switch (draw->base.type)
  {
    case GLX_DRAWABLE_WINDOW:
    {
      winWindowInfoRec winInfo;
      WindowPtr pWin = (WindowPtr) draw->base.pDraw;

      if (!(gc->config->drawableType & GLX_WINDOW_BIT))
        {
          ErrorF("glxWinDeferredCreateContext: tried to attach a context whose fbConfig doesn't have drawableType GLX_WINDOW_BIT to a GLX_DRAWABLE_WINDOW drawable\n");
          // technically, this is an error in the application, but we try to make it work
          // by considering the actual drawable type as well as those specified in the fbConfig
          // when determing the pixel format...
        }

      if (pWin == NULL)
        {
          GLWIN_DEBUG_MSG("Deferring until X window is created");
          return;
        }

      GLWIN_DEBUG_MSG("glxWinDeferredCreateContext: pWin %p", pWin);

      winGetWindowInfo(pWin, &winInfo);
      if (winInfo.hwnd == NULL)
        {
          GLWIN_DEBUG_MSG("Deferring until native window is created");
          return;
        }
    }
    break;

    case GLX_DRAWABLE_PBUFFER:
    {
      if (draw->hPbuffer == NULL)
        {
          // XXX: which DC are supposed to use???
          HDC screenDC = GetDC(NULL);

          if (!(gc->config->drawableType & GLX_PBUFFER_BIT))
            {
              ErrorF("glxWinDeferredCreateContext: tried to attach a context whose fbConfig doesn't have drawableType GLX_PBUFFER_BIT to a GLX_DRAWABLE_PBUFFER drawable\n");
            }

          int pixelFormat = fbConfigToPixelFormatIndex(screenDC, gc->config, GLX_DRAWABLE_PBUFFER);
          if (pixelFormat == 0)
            {
              ErrorF("wglChoosePixelFormat error: %s\n", glxWinErrorMessage());
              return;
            }

          draw->hPbuffer = wglCreatePbufferARBWrapper(screenDC, pixelFormat, draw->base.pDraw->width, draw->base.pDraw->height, NULL);
          ReleaseDC(NULL, screenDC);

          if (draw->hPbuffer == NULL)
            {
              ErrorF("wglCreatePbufferARBWrapper error: %s\n", glxWinErrorMessage());
              return;
            }

          GLWIN_DEBUG_MSG("glxWinDeferredCreateContext: pBuffer %p created for drawable %p", draw->hPbuffer, draw);
        }
    }
    break;

    case GLX_DRAWABLE_PIXMAP:
    {
      if (draw->dibDC == NULL)
        {
          BITMAPINFOHEADER bmpHeader = { sizeof(bmpHeader),
                                         draw->base.pDraw->width, draw->base.pDraw->height,
                                         1, draw->base.pDraw->bitsPerPixel,
                                         BI_RGB};
          void *pBits;

          if (!(gc->config->drawableType & GLX_PIXMAP_BIT))
            {
              ErrorF("glxWinDeferredCreateContext: tried to attach a context whose fbConfig doesn't have drawableType GLX_PIXMAP_BIT to a GLX_DRAWABLE_PIXMAP drawable\n");
            }

          draw->dibDC = CreateCompatibleDC(NULL);
          if (draw->dibDC == NULL)
            {
              ErrorF("CreateCompatibleDC error: %s\n", glxWinErrorMessage());
              return;
            }

          draw->hDIB = CreateDIBSection(draw->dibDC, (BITMAPINFO *)&bmpHeader, DIB_RGB_COLORS, &pBits, 0, 0);
          if (draw->dibDC == NULL)
            {
              ErrorF("CreateDIBSection error: %s\n", glxWinErrorMessage());
              return;
            }

          // XXX: CreateDIBSection insists on allocating the bitmap memory for us, so we're going to
          // need some jiggery pokery to point the underlying X Drawable's bitmap at the same set of bits
          // so that they can be read with XGetImage as well as glReadPixels, assuming the formats are
          // even compatible ...
          draw->pOldBits = ((PixmapPtr)draw->base.pDraw)->devPrivate.ptr;
          ((PixmapPtr)draw->base.pDraw)->devPrivate.ptr = pBits;

          // Select the DIB into the DC
          draw->hOldDIB = SelectObject(draw->dibDC, draw->hDIB);
          if (!draw->hOldDIB)
            {
              ErrorF("SelectObject error: %s\n", glxWinErrorMessage());
            }

          // Set the pixel format of the bitmap
          glxWinSetPixelFormat(gc, draw->dibDC, draw->base.pDraw->bitsPerPixel, GLX_PIXMAP_BIT);

          GLWIN_DEBUG_MSG("glxWinDeferredCreateContext: DIB bitmap %p created for drawable %p", draw->hDIB, draw);
        }
    }
    break;

    default:
    {
      ErrorF("glxWinDeferredCreateContext: tried to attach unhandled drawable type %d\n", draw->base.type);
      return;
    }
  }

  dc = glxWinMakeDC(gc, draw, &dc, &hwnd);
  gc->ctx = wglCreateContext(dc);
  glxWinReleaseDC(hwnd, dc, draw);

  if (gc->ctx == NULL)
    {
      ErrorF("wglCreateContext error: %s\n", glxWinErrorMessage());
      return;
    }

  GLWIN_DEBUG_MSG("glxWinDeferredCreateContext: attached context %p to native context %p pbuffer %p", gc, gc->ctx, draw);

  // if the native context was created successfully, shareLists if needed
  if (gc->ctx && gc->shareContext)
    {
      GLWIN_DEBUG_MSG("glxWinCreateContextReal shareLists with context %p (native ctx %p)", gc->shareContext, gc->shareContext->ctx);

      if (!wglShareLists(gc->shareContext->ctx, gc->ctx))
        {
          ErrorF("wglShareLists error: %s\n", glxWinErrorMessage());
        }
    }
}

/* ---------------------------------------------------------------------- */
/*
 * Context functions
 */


/* Context manipulation routines should return TRUE on success, FALSE on failure */
static int
glxWinContextMakeCurrent(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;
  BOOL ret;
  HDC drawDC;
  HDC readDC = NULL;
  __GLXdrawable *drawPriv;
  __GLXdrawable *readPriv;
  HWND hwnd;

  GLWIN_TRACE_MSG("glxWinContextMakeCurrent context %p (native ctx %p)", gc, gc->ctx);
  glWinCallDelta();

  /* Keep a note of the last active context in the drawable */
  ((__GLXWinDrawable *)(gc->base.drawPriv))->drawContext = gc;

  if (gc->ctx == NULL)
    {
      glxWinDeferredCreateContext(gc, (__GLXWinDrawable *)(gc->base.drawPriv));
    }

  if (gc->ctx == NULL)
    {
      ErrorF("glxWinContextMakeCurrent: Native context is NULL\n");
      return FALSE;
    }

  drawPriv = gc->base.drawPriv;
  drawDC = glxWinMakeDC(gc, (__GLXWinDrawable *)drawPriv, &drawDC, &hwnd);
  if (drawDC == NULL)
    {
      ErrorF("glxWinMakeDC failed for drawDC\n");
      return FALSE;
    }

  /* Otherwise, just use wglMakeCurrent */
  ret = wglMakeCurrent(drawDC, gc->ctx);
  if (!ret)
    {
      ErrorF("wglMakeCurrent error: %s\n", glxWinErrorMessage());
    }

  if ((gc->base.readPriv != NULL) && (gc->base.readPriv != gc->base.drawPriv))
    {
      /*
        If there is a separate read drawable, create a separate read DC, and
        use the wglMakeContextCurrent extension to make the context current drawing
        to one DC and reading from the other

        NOTE WELL: This isn't a strict alternative to wglMakeCurrent() as we can't
        look up the extension function until we have a context, hence the lack of
        an else here (although in practice we cache the look up result for ever...)
      */
      readPriv = gc->base.readPriv;
      readDC = glxWinMakeDC(gc, (__GLXWinDrawable *)readPriv, &readDC, &hwnd);
      if (readDC == NULL)
        {
          ErrorF("glxWinMakeDC failed for readDC\n");
          glxWinReleaseDC(hwnd, drawDC, (__GLXWinDrawable *)readPriv);
          return FALSE;
        }

      ret = wglMakeContextCurrentARBWrapper(drawDC, readDC, gc->ctx);
      if (!ret)
        {
          ErrorF("wglMakeContextCurrent error: %s\n", glxWinErrorMessage());
        }
    }

  // apparently make current could fail if the context is current in a different thread,
  // but that shouldn't be able to happen in the current server...

  glxWinReleaseDC(hwnd, drawDC, (__GLXWinDrawable *)drawPriv);
  if (readDC)
    glxWinReleaseDC(hwnd, drawDC, (__GLXWinDrawable *)readPriv);

  return ret;
}

static int
glxWinContextLoseCurrent(__GLXcontext *base)
{
  BOOL ret;
  __GLXWinContext *gc = (__GLXWinContext *)base;

  GLWIN_TRACE_MSG("glxWinContextLoseCurrent context %p (native ctx %p)", gc, gc->ctx);
  glWinCallDelta();

  ret = wglMakeCurrent(NULL, NULL); /* We don't need a DC when setting no current context */
  if (!ret)
    ErrorF("glxWinContextLoseCurrent error: %s\n", glxWinErrorMessage());

  __glXLastContext = NULL; /* Mesa does this; why? */

  return TRUE;
}

static int
glxWinContextCopy(__GLXcontext *dst_base, __GLXcontext *src_base, unsigned long mask)
{
  __GLXWinContext *dst = (__GLXWinContext *)dst_base;
  __GLXWinContext *src = (__GLXWinContext *)src_base;
  BOOL ret;

  GLWIN_DEBUG_MSG("glxWinContextCopy");

  ret = wglCopyContext(src->ctx, dst->ctx, mask);
  if (!ret)
    {
      ErrorF("wglCopyContext error: %s\n", glxWinErrorMessage());
    }

  return ret;
}

static int
glxWinContextForceCurrent(__GLXcontext *base)
{
  /* wglMakeCurrent always flushes the previous context, so this is equivalent to glxWinContextMakeCurrent */
  return glxWinContextMakeCurrent(base);
}

static void
glxWinContextDestroy(__GLXcontext *base)
{
  __GLXWinContext *gc = (__GLXWinContext *)base;

  if (gc != NULL)
    {
      if (gc->ctx)
        {
          BOOL ret = wglDeleteContext(gc->ctx);
          if (!ret)
            ErrorF("wglDeleteContext error: %s\n", glxWinErrorMessage());
          gc->ctx = NULL;
        }

      GLWIN_DEBUG_MSG("GLXcontext %p destroyed (native ctx %p)", base, gc->ctx);

      xfree(gc);
    }
}

static __GLXcontext *
glxWinCreateContext(__GLXscreen *screen,
                   __GLXconfig *modes,
                   __GLXcontext *baseShareContext)
{
    __GLXWinContext *context;
    __GLXWinContext *shareContext = (__GLXWinContext *)baseShareContext;

    static __GLXtextureFromPixmap glxWinTextureFromPixmap =
      {
        glxWinBindTexImage,
        glxWinReleaseTexImage
      };

    context = (__GLXWinContext *)xcalloc(1, sizeof(__GLXWinContext));

    if (!context)
        return NULL;

    memset(context, 0, sizeof *context);
    context->base.destroy        = glxWinContextDestroy;
    context->base.makeCurrent    = glxWinContextMakeCurrent;
    context->base.loseCurrent    = glxWinContextLoseCurrent;
    context->base.copy           = glxWinContextCopy;
    context->base.forceCurrent   = glxWinContextForceCurrent;
    context->base.textureFromPixmap = &glxWinTextureFromPixmap;
    context->base.config = modes;
    context->base.pGlxScreen = screen;

    // actual native GL context creation is deferred until attach()
    context->ctx = NULL;
    context->shareContext = shareContext;
    context->config = modes;

    glWinSetupDispatchTable();

    GLWIN_DEBUG_MSG("GLXcontext %p created", context);

    return &(context->base);
}

/* ---------------------------------------------------------------------- */
/*
 * Utility functions
 */

static int
fbConfigToPixelFormat(__GLXconfig *mode, PIXELFORMATDESCRIPTOR *pfdret, int drawableTypeOverride)
{
    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),   /* size of this pfd */
      1,                     /* version number */
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

    if ((mode->drawableType | drawableTypeOverride) & GLX_WINDOW_BIT)
      pfd.dwFlags |= PFD_DRAW_TO_WINDOW; /* support window */

    if ((mode->drawableType | drawableTypeOverride) & GLX_PIXMAP_BIT)
      pfd.dwFlags |= (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI); /* supports software rendering to bitmap */

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

    /* mode->level ? */
    /* mode->pixmapMode ? */

    *pfdret = pfd;

    return 0;
}

#define SET_ATTR_VALUE(attr, value) { attribList[i++] = attr; attribList[i++] = value; }

static int
fbConfigToPixelFormatIndex(HDC hdc, __GLXconfig *mode, int drawableTypeOverride)
{
  UINT numFormats;
  unsigned int i = 0;
  static UINT maxFormats = 0;

  if (maxFormats == 0)
    {
      const int attr = WGL_NUMBER_PIXEL_FORMATS_ARB;

      if (!wglGetPixelFormatAttribivARBWrapper(hdc, 0, 0, 1, &attr, &maxFormats))
        {
          ErrorF("wglGetPixelFormatAttribivARB failed for WGL_NUMBER_PIXEL_FORMATS_ARB: %s\n", glxWinErrorMessage());
          return 0;
        }
    }

  /* convert fbConfig to attr-value list  */
  // XXX: and remember we are not allowed to ask for unsupported attrs
  int attribList[] = {
    WGL_SUPPORT_OPENGL_ARB, TRUE,
    WGL_PIXEL_TYPE_ARB, (mode->visualType == GLX_TRUE_COLOR) ? WGL_TYPE_RGBA_ARB : WGL_TYPE_COLORINDEX_ARB,
    WGL_COLOR_BITS_ARB, (mode->visualType == GLX_TRUE_COLOR) ? mode->rgbBits : mode->indexBits,
    WGL_RED_BITS_ARB, mode->redBits,
    WGL_GREEN_BITS_ARB, mode->greenBits,
    WGL_BLUE_BITS_ARB, mode->blueBits,
    WGL_ALPHA_BITS_ARB, mode->alphaBits,
    WGL_ACCUM_RED_BITS_ARB, mode->accumRedBits,
    WGL_ACCUM_GREEN_BITS_ARB, mode->accumGreenBits,
    WGL_ACCUM_BLUE_BITS_ARB, mode->accumBlueBits,
    WGL_ACCUM_ALPHA_BITS_ARB, mode->accumAlphaBits,
    WGL_DEPTH_BITS_ARB, mode->depthBits,
    WGL_STENCIL_BITS_ARB, mode->stencilBits,
    WGL_AUX_BUFFERS_ARB, mode->numAuxBuffers,
  };

  if (mode->doubleBufferMode)
    SET_ATTR_VALUE(WGL_DOUBLE_BUFFER_ARB, TRUE);

  if (mode->stereoMode)
    SET_ATTR_VALUE(WGL_STEREO_ARB, TRUE);

  // Some attributes are only added to the list if the value requested is not 'don't care', as exactly matching that is daft..
  if (mode->swapMethod == GLX_SWAP_EXCHANGE_OML)
    SET_ATTR_VALUE(WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB);

  if (mode->swapMethod == GLX_SWAP_COPY_OML)
    SET_ATTR_VALUE(WGL_SWAP_COPY_ARB, TRUE);

  // XXX: this should probably be the other way around, but that messes up drawableTypeOverride
  if (mode->visualRating == GLX_SLOW_VISUAL_EXT)
    SET_ATTR_VALUE(WGL_ACCELERATION_ARB, WGL_NO_ACCELERATION_ARB);

  // must support all the drawable types the mode supports
  if ((mode->drawableType | drawableTypeOverride) & GLX_WINDOW_BIT)
    SET_ATTR_VALUE(WGL_DRAW_TO_WINDOW_ARB,TRUE);

  // XXX: this is a horrible hacky heuristic, in fact this whole drawableTypeOverride thing is a bad idea
  // try to avoid asking for formats which don't exist (by not asking for all when adjusting the config to include the drawableTypeOverride)
  if (drawableTypeOverride == GLX_WINDOW_BIT)
    {
      if (mode->drawableType & GLX_PIXMAP_BIT)
        SET_ATTR_VALUE(WGL_DRAW_TO_BITMAP_ARB, TRUE);

      if (mode->drawableType & GLX_PBUFFER_BIT)
        SET_ATTR_VALUE(WGL_DRAW_TO_PBUFFER_ARB, TRUE);
    }
  else
    {
      if (drawableTypeOverride & GLX_PIXMAP_BIT)
        SET_ATTR_VALUE(WGL_DRAW_TO_BITMAP_ARB, TRUE);

      if (drawableTypeOverride & GLX_PBUFFER_BIT)
        SET_ATTR_VALUE(WGL_DRAW_TO_PBUFFER_ARB, TRUE);
    }

  SET_ATTR_VALUE(0, 0); // terminator

  /* choose the first match */
  {
    int pixelFormatIndex[maxFormats];

    if (!wglChoosePixelFormatARBWrapper(hdc, attribList, NULL, maxFormats, pixelFormatIndex, &numFormats))
      {
        ErrorF("wglChoosePixelFormat error: %s\n", glxWinErrorMessage());
      }
    else
      {
        if (numFormats > 0)
          {
            unsigned int i;
            GLWIN_DEBUG_MSG("wglChoosePixelFormat: chose pixelFormatIndex %d (%d possibles)", pixelFormatIndex[0], numFormats);
            /* for (i = 0; i < numFormats; i++) */
            /*   GLWIN_DEBUG_MSG("wglChoosePixelFormat: %d", pixelFormatIndex[i]); */

            return pixelFormatIndex[0];
          }
        else
          ErrorF("wglChoosePixelFormat couldn't decide\n");
      }
  }

  return 0;
}

/* ---------------------------------------------------------------------- */

#define BITS_AND_SHIFT_TO_MASK(bits,mask) (((1<<(bits))-1) << (mask))

//
// Create the GLXconfigs using DescribePixelFormat()
//
static __GLXconfig *
glxWinCreateConfigs(HDC hdc, int *numConfigsPtr, int screenNumber)
{
  __GLXconfig *c, *result, *prev = NULL;
  int numConfigs = 0;
  int i = 0;
  int n = 0;
  PIXELFORMATDESCRIPTOR pfd;

  GLWIN_DEBUG_MSG("glxWinCreateConfigs");

  // get the number of pixelformats
  numConfigs = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
  GLWIN_DEBUG_MSG("DescribePixelFormat says %d possible pixel formats", numConfigs);

  /* alloc */
  result = xalloc(sizeof(__GLXconfig) * numConfigs);

  if (NULL == result)
    {
      *numConfigsPtr = 0;
      return NULL;
    }

  memset(result, 0, sizeof(__GLXconfig) * numConfigs);
  n = 0;

  /* fill in configs */
  for (i = 0;  i < numConfigs; i++)
    {
      int rc;

      c = &(result[i]);
      c->next = NULL;

      rc = DescribePixelFormat(hdc, i+1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

      if (!rc)
        {
          ErrorF("DescribePixelFormat failed for index %d, error %s\n", i+1, glxWinErrorMessage());
          break;
        }

      if (!(pfd.dwFlags & (PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP)) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL))
        {
          GLWIN_DEBUG_MSG("pixelFormat %d has unsuitable flags 0x%08lx, skipping", i+1, pfd.dwFlags);
          continue;
        }

      if (glxWinDebugSettings.dumpPFD)
        pfdOut(&pfd);

      c->doubleBufferMode = (pfd.dwFlags & PFD_DOUBLEBUFFER) ? GL_TRUE : GL_FALSE;
      c->stereoMode = (pfd.dwFlags & PFD_STEREO) ? GL_TRUE : GL_FALSE;

      c->redBits = pfd.cRedBits;
      c->greenBits = pfd.cGreenBits;
      c->blueBits = pfd.cBlueBits;
      c->alphaBits = pfd.cAlphaBits;

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
      c->level = 0;
      // pfd.dwLayerMask; // ignored
      // pfd.dwDamageMask;  // ignored

      c->pixmapMode = 0;
      c->visualID = -1;  // will be set by __glXScreenInit()

      /* EXT_visual_rating / GLX 1.2 */
      if (pfd.dwFlags & PFD_GENERIC_FORMAT)
        {
          c->visualRating = GLX_SLOW_VISUAL_EXT;
        }
      else
        {
          // PFD_GENERIC_ACCELERATED is not considered, so this may be MCD or ICD acclerated...
          c->visualRating = GLX_NONE_EXT;
        }

      /* EXT_visual_info / GLX 1.2 */
      if (pfd.iPixelType == PFD_TYPE_COLORINDEX)
        {
          c->visualType = GLX_STATIC_COLOR;
        }
      else
        {
          c->visualType = GLX_TRUE_COLOR;
        }

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
      c->drawableType = (((pfd.dwFlags & PFD_DRAW_TO_WINDOW) ? GLX_WINDOW_BIT : 0)
                         | ((pfd.dwFlags & PFD_DRAW_TO_BITMAP) ? GLX_PIXMAP_BIT : 0));
      c->renderType = GLX_RGBA_BIT;
      c->xRenderable = GL_TRUE;
      c->fbconfigID = -1; // will be set by __glXScreenInit()

      /* SGIX_pbuffer / GLX 1.3 */
      // XXX: How can we find these values out ???
      c->maxPbufferWidth = -1;
      c->maxPbufferHeight = -1;
      c->maxPbufferPixels = -1;
      c->optimalPbufferWidth = 0; // there is no optimal value
      c->optimalPbufferHeight = 0;

      /* SGIX_visual_select_group */
      // arrange for visuals with the best acceleration to be preferred in selection
      switch (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED))
        {
        case 0:
          c->visualSelectGroup = 2;
          break;

        case PFD_GENERIC_ACCELERATED:
          c->visualSelectGroup = 1;
          break;

        case PFD_GENERIC_FORMAT:
          c->visualSelectGroup = 0;
          break;

        default:
          ;
          // "can't happen"
        }

      /* OML_swap_method */
      if (pfd.dwFlags & PFD_SWAP_EXCHANGE)
        c->swapMethod = GLX_SWAP_EXCHANGE_OML;
      else if (pfd.dwFlags & PFD_SWAP_COPY)
        c->swapMethod = GLX_SWAP_COPY_OML;
      else
        c->swapMethod = GLX_SWAP_UNDEFINED_OML;

      /* EXT_import_context */
      c->screen = screenNumber;

      /* EXT_texture_from_pixmap */
      c->bindToTextureRgb = -1; // this is important for swrast to match these fbConfigs
      c->bindToTextureRgba = -1;
      c->bindToMipmapTexture = -1;
      c->bindToTextureTargets = -1;
      c->yInverted = -1;

      n++;

      // update previous config to point to this config
      if (prev)
        prev->next = c;

      prev = c;
    }

  GLWIN_DEBUG_MSG("found %d pixelFormats suitable for conversion to fbConfigs", n);
  *numConfigsPtr = n;
  return result;
}

// helper function to access an attribute value from an attribute value array by attribute
static
int getAttrValue(const int attrs[], int values[], unsigned int num, int attr, int fallback)
{
  unsigned int i;
  for (i = 0; i < num; i++)
    {
      if (attrs[i] == attr)
        {
          GLWIN_TRACE_MSG("getAttrValue attr 0x%x, value %d", attr, values[i]);
          return values[i];
        }
    }

  ErrorF("getAttrValue failed to find attr 0x%x, using default value %d\n", attr, fallback);
  return fallback;
}

#define NUM_ELEMENTS(x) (sizeof(x)/ sizeof(x[1]))

//
// Create the GLXconfigs using wglGetPixelFormatAttribfvARB() extension
//
static __GLXconfig *
glxWinCreateConfigsExt(HDC hdc, int *numConfigsPtr, int screenNumber)
{
  __GLXconfig *c, *result, *prev = NULL;
  int i = 0;
  int n = 0;

  const int attr = WGL_NUMBER_PIXEL_FORMATS_ARB;
  int numConfigs;

  GLWIN_DEBUG_MSG("glxWinCreateConfigsExt");

  if (!wglGetPixelFormatAttribivARBWrapper(hdc, 0, 0, 1, &attr, &numConfigs))
    {
      ErrorF("wglGetPixelFormatAttribivARB failed for WGL_NUMBER_PIXEL_FORMATS_ARB: %s\n", glxWinErrorMessage());
      return NULL;
    }

  GLWIN_DEBUG_MSG("wglGetPixelFormatAttribivARB says %d possible pixel formats", numConfigs);

  /* alloc */
  result = xalloc(sizeof(__GLXconfig) * numConfigs);

  if (NULL == result)
    {
      *numConfigsPtr = 0;
      return NULL;
    }

  memset(result, 0, sizeof(__GLXconfig) * numConfigs);
  n = 0;

  /* fill in configs */
  for (i = 0;  i < numConfigs; i++)
    {
      c = &(result[i]);
      c->next = NULL;

      const int attrs[] =
        {
          WGL_DRAW_TO_WINDOW_ARB,
          WGL_DRAW_TO_BITMAP_ARB,
          WGL_ACCELERATION_ARB,
          WGL_SWAP_LAYER_BUFFERS_ARB,
          WGL_NUMBER_OVERLAYS_ARB,
          WGL_NUMBER_UNDERLAYS_ARB,
          WGL_TRANSPARENT_ARB,
          WGL_TRANSPARENT_RED_VALUE_ARB,
          WGL_TRANSPARENT_GREEN_VALUE_ARB,
          WGL_TRANSPARENT_GREEN_VALUE_ARB,
          WGL_TRANSPARENT_ALPHA_VALUE_ARB,
          WGL_SUPPORT_OPENGL_ARB,
          WGL_DOUBLE_BUFFER_ARB,
          WGL_STEREO_ARB,
          WGL_PIXEL_TYPE_ARB,
          WGL_COLOR_BITS_ARB,
          WGL_RED_BITS_ARB,
          WGL_RED_SHIFT_ARB,
          WGL_GREEN_BITS_ARB,
          WGL_GREEN_SHIFT_ARB,
          WGL_BLUE_BITS_ARB,
          WGL_BLUE_SHIFT_ARB,
          WGL_ALPHA_BITS_ARB,
          WGL_ALPHA_SHIFT_ARB,
          WGL_ACCUM_RED_BITS_ARB,
          WGL_ACCUM_GREEN_BITS_ARB,
          WGL_ACCUM_BLUE_BITS_ARB,
          WGL_ACCUM_ALPHA_BITS_ARB,
          WGL_DEPTH_BITS_ARB,
          WGL_STENCIL_BITS_ARB,
          WGL_AUX_BUFFERS_ARB,
          WGL_SWAP_METHOD_ARB,
          // XXX: we may not query these attrs if WGL_ARB_multisample is not offered...
          WGL_SAMPLE_BUFFERS_ARB,
          WGL_SAMPLES_ARB,
          // XXX: we may not query these attrs if WGL_ARB_render_texture is not offered...
          WGL_BIND_TO_TEXTURE_RGB_ARB,
          WGL_BIND_TO_TEXTURE_RGBA_ARB,
          // XXX: we may not query these attrs if WGL_ARB_pbuffer is not offered...
          WGL_DRAW_TO_PBUFFER_ARB,
          WGL_MAX_PBUFFER_PIXELS_ARB,
          WGL_MAX_PBUFFER_WIDTH_ARB,
          WGL_MAX_PBUFFER_HEIGHT_ARB,
        };

      unsigned int num_attrs = NUM_ELEMENTS(attrs);
      int values[num_attrs];

      if (!wglGetPixelFormatAttribivARBWrapper(hdc, i+1, 0, num_attrs, attrs, values))
        {
          ErrorF("wglGetPixelFormatAttribivARB failed for index %d, error %s\n", i+1, glxWinErrorMessage());
          break;
        }

#define ATTR_VALUE(a, d) getAttrValue(attrs, values, num_attrs, (a), (d))

      if (!ATTR_VALUE(WGL_SUPPORT_OPENGL_ARB, 0))
        {
          GLWIN_DEBUG_MSG("pixelFormat %d isn't WGL_SUPPORT_OPENGL_ARB, skipping", i+1);
          continue;
        }

      c->doubleBufferMode = ATTR_VALUE(WGL_DOUBLE_BUFFER_ARB, 0) ? GL_TRUE : GL_FALSE;
      c->stereoMode = ATTR_VALUE(WGL_STEREO_ARB, 0) ? GL_TRUE : GL_FALSE;

      c->redBits = ATTR_VALUE(WGL_RED_BITS_ARB, 0);
      c->greenBits = ATTR_VALUE(WGL_GREEN_BITS_ARB, 0);
      c->blueBits = ATTR_VALUE(WGL_BLUE_BITS_ARB, 0);
      c->alphaBits = ATTR_VALUE(WGL_ALPHA_BITS_ARB, 0);

      c->redMask = BITS_AND_SHIFT_TO_MASK(c->redBits, ATTR_VALUE(WGL_RED_SHIFT_ARB, 0));
      c->greenMask = BITS_AND_SHIFT_TO_MASK(c->greenBits, ATTR_VALUE(WGL_GREEN_SHIFT_ARB, 0));
      c->blueMask = BITS_AND_SHIFT_TO_MASK(c->blueBits, ATTR_VALUE(WGL_BLUE_SHIFT_ARB, 0));
      c->alphaMask = BITS_AND_SHIFT_TO_MASK(c->alphaBits, ATTR_VALUE(WGL_ALPHA_SHIFT_ARB, 0));

      switch (ATTR_VALUE(WGL_PIXEL_TYPE_ARB, 0))
        {
        case WGL_TYPE_COLORINDEX_ARB:
          c->indexBits = ATTR_VALUE(WGL_COLOR_BITS_ARB, 0);
          c->rgbBits = 0;
          c->visualType = GLX_STATIC_COLOR;
          break;

        case WGL_TYPE_RGBA_FLOAT_ARB:
          GLWIN_DEBUG_MSG("pixelFormat %d is WGL_TYPE_RGBA_FLOAT_ARB, skipping", i+1);
          continue;

        case WGL_TYPE_RGBA_ARB:
          c->indexBits = 0;
          c->rgbBits = ATTR_VALUE(WGL_COLOR_BITS_ARB, 0);
          c->visualType = GLX_TRUE_COLOR;
          break;

        default:
          ErrorF("wglGetPixelFormatAttribivARB returned unknown value 0x%x for WGL_PIXEL_TYPE_ARB\n", ATTR_VALUE(WGL_PIXEL_TYPE_ARB, 0));
          continue;
        }

      c->accumRedBits = ATTR_VALUE(WGL_ACCUM_RED_BITS_ARB, 0);
      c->accumGreenBits = ATTR_VALUE(WGL_ACCUM_GREEN_BITS_ARB, 0);
      c->accumBlueBits = ATTR_VALUE(WGL_ACCUM_BLUE_BITS_ARB, 0);
      c->accumAlphaBits = ATTR_VALUE(WGL_ACCUM_ALPHA_BITS_ARB, 0);

      c->depthBits = ATTR_VALUE(WGL_DEPTH_BITS_ARB, 0);
      c->stencilBits = ATTR_VALUE(WGL_STENCIL_BITS_ARB, 0);
      c->numAuxBuffers = ATTR_VALUE(WGL_AUX_BUFFERS_ARB, 0);

      {
        int layers = ATTR_VALUE(WGL_NUMBER_OVERLAYS_ARB,0) + ATTR_VALUE(WGL_NUMBER_UNDERLAYS_ARB, 0);

        if (layers > 0)
          {
            ErrorF("pixelFormat %d: has %d overlay, %d underlays which aren't currently handled", i, ATTR_VALUE(WGL_NUMBER_OVERLAYS_ARB,0), ATTR_VALUE(WGL_NUMBER_UNDERLAYS_ARB, 0));
            // XXX: need to iterate over layers?
          }
      }
      c->level = 0;

      c->pixmapMode = 0; // ???
      c->visualID = -1;  // will be set by __glXScreenInit()

      /* EXT_visual_rating / GLX 1.2 */
      switch (ATTR_VALUE(WGL_ACCELERATION_ARB, 0))
        {
        default:
          ErrorF("wglGetPixelFormatAttribivARB returned unknown value 0x%x for WGL_ACCELERATION_ARB\n", ATTR_VALUE(WGL_ACCELERATION_ARB, 0));

        case WGL_NO_ACCELERATION_ARB:
          c->visualRating = GLX_SLOW_VISUAL_EXT;
          break;

        case WGL_GENERIC_ACCELERATION_ARB:
        case WGL_FULL_ACCELERATION_ARB:
          c->visualRating = GLX_NONE_EXT;
          break;
        }

      /* EXT_visual_info / GLX 1.2 */
      // c->visualType is set above
      if (ATTR_VALUE(WGL_TRANSPARENT_ARB, 0))
        {
          c->transparentPixel = (c->visualType == GLX_TRUE_COLOR) ? GLX_TRANSPARENT_RGB_EXT : GLX_TRANSPARENT_INDEX_EXT;
          c->transparentRed = ATTR_VALUE(WGL_TRANSPARENT_RED_VALUE_ARB, 0);
          c->transparentGreen = ATTR_VALUE(WGL_TRANSPARENT_GREEN_VALUE_ARB, 0);
          c->transparentBlue = ATTR_VALUE(WGL_TRANSPARENT_BLUE_VALUE_ARB, 0);
          c->transparentAlpha = ATTR_VALUE(WGL_TRANSPARENT_ALPHA_VALUE_ARB, 0);
          c->transparentIndex = ATTR_VALUE(WGL_TRANSPARENT_INDEX_VALUE_ARB, 0);
        }
      else
        {
          c->transparentPixel = GLX_NONE_EXT;
          c->transparentRed = GLX_NONE;
          c->transparentGreen = GLX_NONE;
          c->transparentBlue = GLX_NONE;
          c->transparentAlpha = GLX_NONE;
          c->transparentIndex = GLX_NONE;
        }

      /* ARB_multisample / SGIS_multisample */
      // XXX: we may not query these attrs if WGL_ARB_multisample is not offered...
      c->sampleBuffers = ATTR_VALUE(WGL_SAMPLE_BUFFERS_ARB, 0);
      c->samples = ATTR_VALUE(WGL_SAMPLES_ARB, 0);

      /* SGIX_fbconfig / GLX 1.3 */
      c->drawableType = ((ATTR_VALUE(WGL_DRAW_TO_WINDOW_ARB, 0) ? GLX_WINDOW_BIT : 0)
                         | (ATTR_VALUE(WGL_DRAW_TO_BITMAP_ARB, 0) ? GLX_PIXMAP_BIT : 0)
                         | (ATTR_VALUE(WGL_DRAW_TO_PBUFFER_ARB, 0) ? GLX_PBUFFER_BIT : 0));
      c->renderType = GLX_RGBA_BIT | GLX_COLOR_INDEX_BIT; // Hmmm ???
      c->xRenderable = GL_TRUE;
      c->fbconfigID = -1; // will be set by __glXScreenInit()

      /* SGIX_pbuffer / GLX 1.3 */
      c->maxPbufferWidth = ATTR_VALUE(WGL_MAX_PBUFFER_WIDTH_ARB, -1);
      c->maxPbufferHeight = ATTR_VALUE(WGL_MAX_PBUFFER_HEIGHT_ARB, -1);
      c->maxPbufferPixels =  ATTR_VALUE(WGL_MAX_PBUFFER_PIXELS_ARB, -1);
      c->optimalPbufferWidth = 0;
      c->optimalPbufferHeight = 0;

      /* SGIX_visual_select_group */
      // arrange for visuals with the best acceleration to be preferred in selection
      switch (ATTR_VALUE(WGL_ACCELERATION_ARB, 0))
        {
        case WGL_FULL_ACCELERATION_ARB:
          c->visualSelectGroup = 2;
          break;

        case WGL_GENERIC_ACCELERATION_ARB:
          c->visualSelectGroup = 1;
          break;

        default:
        case WGL_NO_ACCELERATION_ARB:
          c->visualSelectGroup = 0;
          break;
        }

      /* OML_swap_method */
      switch (ATTR_VALUE(WGL_SWAP_METHOD_ARB, 0))
        {
        case WGL_SWAP_EXCHANGE_ARB:
          c->swapMethod = GLX_SWAP_EXCHANGE_OML;
          break;

        case WGL_SWAP_COPY_ARB:
          c->swapMethod = GLX_SWAP_COPY_OML;
          break;

        default:
          ErrorF("wglGetPixelFormatAttribivARB returned unknown value 0x%x for WGL_SWAP_METHOD_ARB\n", ATTR_VALUE(WGL_SWAP_METHOD_ARB, 0));

        case WGL_SWAP_UNDEFINED_ARB:
          c->swapMethod = GLX_SWAP_UNDEFINED_OML;
        }

      /* EXT_import_context */
      c->screen = screenNumber;

      /* EXT_texture_from_pixmap */
      c->bindToTextureRgb = ATTR_VALUE(WGL_BIND_TO_TEXTURE_RGB_ARB, 0);
      c->bindToTextureRgba = ATTR_VALUE(WGL_BIND_TO_TEXTURE_RGBA_ARB, 0);;
      c->bindToMipmapTexture = -1;
      c->bindToTextureTargets = -1;
      c->yInverted = -1;

      n++;

      // update previous config to point to this config
      if (prev)
        prev->next = c;

      prev = c;
    }

  *numConfigsPtr = n;

  return result;
}

