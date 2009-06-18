/*
 * File: glwrap.c
 * Purpose: Wrapper functions for Win32 OpenGL functions
 *
 * Authors: Alexander Gottwald
 *          Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 * Copyright (c) Alexander Gottwald 2004
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

#define USE_OPENGL32

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glx/glxserver.h>
#include <glx/glxext.h>
#include <glx/glapi.h>
#include <glx/dispatch.h>
#include <glwindows.h>

unsigned int glWinIndirectProcCalls = 0;
unsigned int glWinDirectProcCalls = 0;

void
glWinCallDelta(void)
{
  static unsigned int glWinIndirectProcCallsLast = 0;
  static unsigned int glWinDirectProcCallsLast = 0;
  if ((glWinIndirectProcCalls != glWinIndirectProcCallsLast) ||
      (glWinDirectProcCalls != glWinDirectProcCallsLast))
    {
      ErrorF("after %d direct and %d indirect GL calls\n",
             glWinDirectProcCalls - glWinDirectProcCallsLast,
             glWinIndirectProcCalls - glWinIndirectProcCallsLast);
      glWinDirectProcCallsLast = glWinDirectProcCalls;
      glWinIndirectProcCallsLast = glWinIndirectProcCalls;
    }
}

/*
 * Not sure why these typedefs aren't provided by gl.h/glext.h...
 */
typedef void (APIENTRYP PFNGLBLENDEQUATIONPROC) (GLenum mode);
typedef void (APIENTRYP PFNGLBLENDCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRYP PFNGLSAMPLECOVERAGEPROC) (GLclampf value, GLboolean invert);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE3DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE1DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLGETCOMPRESSEDTEXIMAGEPROC) (GLenum target, GLint level, GLvoid *img);
#define PFNGLDELETEOBJECTBUFFERATIPROC PFNGLFREEOBJECTBUFFERATIPROC

#define RESOLVE_RET(procname, symbol, retval) \
    static Bool init = TRUE; \
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
    } \
    glWinIndirectProcCalls++;

#define RESOLVE(procname, symbol) RESOLVE_RET(procname, symbol,)

/*
 *  cdecl wrappers for stdcall gl*() functions in opengl32.dll
 */

/*
   OpenGL 1.2 and upward is treated as extensions, function address must
   found using wglGetProcAddress() - still stdcall so still need wrappers...
 */

#include "glwrap_api.c"

/*
  Special non-static wrapper for glGetString for debug output
*/

const GLubyte* glGetStringWrapperNonstatic(GLenum name)
{
  return glGetString(name);
}

/*
 *
 */

static
void glGetProgramivARBWrapper(GLenum target, GLenum pname, GLint *params)
{
  RESOLVE(PFNGLGETPROGRAMIVARBPROC, "glGetProgramivARB");
  proc(target, pname, params);
}

static void glBlendFuncSeparateWrapper(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    RESOLVE(PFNGLBLENDFUNCSEPARATEPROC, "glBlendFuncSeparate");
    proc(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
static void glFogCoordfvWrapper(const GLfloat *coord)
{
    RESOLVE(PFNGLFOGCOORDFVPROC, "glFogCoordfv");
    proc(coord);
}
static void glFogCoorddvWrapper(const GLdouble *coord)
{
    RESOLVE(PFNGLFOGCOORDDVPROC, "glFogCoorddv");
    proc(coord);
}
static void glFogCoordPointerWrapper(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    RESOLVE(PFNGLFOGCOORDPOINTERPROC, "glFogCoordPointer");
    proc(type, stride, pointer);
}

static void glSecondaryColor3bvWrapper(const GLbyte *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3BVPROC, "glSecondaryColor3bv");
    proc(v);
}
static void glSecondaryColor3dvWrapper(const GLdouble *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3DVPROC, "glSecondaryColor3dv");
    proc(v);
}
static void glSecondaryColor3fvWrapper(const GLfloat *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3FVPROC, "glSecondaryColor3fv");
    proc(v);
}
static void glSecondaryColor3ivWrapper(const GLint *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3IVPROC, "glSecondaryColor3iv");
    proc(v);
}
static void glSecondaryColor3svWrapper(const GLshort *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3SVPROC, "glSecondaryColor3sv");
    proc(v);
}
static void glSecondaryColor3ubvWrapper(const GLubyte *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3UBVPROC, "glSecondaryColor3ubv");
    proc(v);
}
static void glSecondaryColor3uivWrapper(const GLuint *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3UIVPROC, "glSecondaryColor3uiv");
    proc(v);
}
static void glSecondaryColor3usvWrapper(const GLushort *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3USVPROC, "glSecondaryColor3usv");
    proc(v);
}
static void glSecondaryColorPointerWrapper(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    RESOLVE(PFNGLSECONDARYCOLORPOINTERPROC, "glSecondaryColorPointer");
    proc(size, type, stride, pointer);
}

static void glActiveStencilFaceEXTWrapper(GLenum face)
{
    RESOLVE(PFNGLACTIVESTENCILFACEEXTPROC, "glActiveStencilFaceEXT");
    proc(face);
}

/*
 *
 */

static void
warn_func(void * ctx, const char *format, ...)
{
    va_list v;
    va_start(v, format);
    vfprintf(stderr, format, v);
    va_end(v);
    fprintf(stderr,"\n");
}

static void
glx_debugging(void)
{
  _glapi_set_warning_func(warn_func);
  _glapi_noop_enable_warnings(TRUE);
}

void setup_dispatch_table(void)
{
  struct _glapi_table *disp=_glapi_get_dispatch();
  glx_debugging();

  SET_GetString(disp, glGetStringWrapper);
  SET_GetIntegerv(disp, glGetIntegervWrapper);

  SET_GenTextures(disp, glGenTexturesWrapper);
  SET_DeleteTextures(disp, glDeleteTexturesWrapper);
  SET_BindTexture(disp, glBindTextureWrapper);
  SET_PrioritizeTextures(disp, glPrioritizeTexturesWrapper);
  SET_AreTexturesResident(disp, glAreTexturesResidentWrapper);
  SET_IsTexture(disp, glIsTextureWrapper);

  SET_Accum(disp, glAccumWrapper);
  SET_AlphaFunc(disp, glAlphaFuncWrapper);

  SET_ArrayElement(disp, glArrayElementWrapper);
  SET_Begin(disp, glBeginWrapper);

  SET_Bitmap(disp, glBitmapWrapper);
  SET_BlendColor(disp, glBlendColorWrapper);
  SET_BlendEquation(disp, glBlendEquationWrapper);
  SET_BlendFunc(disp, glBlendFuncWrapper);
  SET_CallList(disp, glCallListWrapper);
  SET_CallLists(disp, glCallListsWrapper);
  SET_Clear(disp, glClearWrapper);
  SET_ClearAccum(disp, glClearAccumWrapper);
  SET_ClearColor(disp, glClearColorWrapper);
  SET_ClearDepth(disp, glClearDepthWrapper);
  SET_ClearIndex(disp, glClearIndexWrapper);
  SET_ClearStencil(disp, glClearStencilWrapper);
  SET_ClipPlane(disp, glClipPlaneWrapper);
  SET_Color3b(disp, glColor3bWrapper);
  SET_Color3bv(disp, glColor3bvWrapper);
  SET_Color3d(disp, glColor3dWrapper);
  SET_Color3dv(disp, glColor3dvWrapper);
  SET_Color3f(disp, glColor3fWrapper);
  SET_Color3fv(disp, glColor3fvWrapper);
  SET_Color3i(disp, glColor3iWrapper);
  SET_Color3iv(disp, glColor3ivWrapper);
  SET_Color3s(disp, glColor3sWrapper);
  SET_Color3sv(disp, glColor3svWrapper);
  SET_Color3ub(disp, glColor3ubWrapper);
  SET_Color3ubv(disp, glColor3ubvWrapper);
  SET_Color3ui(disp, glColor3uiWrapper);
  SET_Color3uiv(disp, glColor3uivWrapper);
  SET_Color3us(disp, glColor3usWrapper);
  SET_Color3usv(disp, glColor3usvWrapper);
  SET_Color4b(disp, glColor4bWrapper);
  SET_Color4bv(disp, glColor4bvWrapper);
  SET_Color4d(disp, glColor4dWrapper);
  SET_Color4dv(disp, glColor4dvWrapper);
  SET_Color4f(disp, glColor4fWrapper);
  SET_Color4fv(disp, glColor4fvWrapper);
  SET_Color4i(disp, glColor4iWrapper);
  SET_Color4iv(disp, glColor4ivWrapper);
  SET_Color4s(disp, glColor4sWrapper);
  SET_Color4sv(disp, glColor4svWrapper);
  SET_Color4ub(disp, glColor4ubWrapper);
  SET_Color4ubv(disp, glColor4ubvWrapper);
  SET_Color4ui(disp, glColor4uiWrapper);
  SET_Color4uiv(disp, glColor4uivWrapper);
  SET_Color4us(disp, glColor4usWrapper);
  SET_Color4usv(disp, glColor4usvWrapper);
  SET_ColorMask(disp, glColorMaskWrapper);
  SET_ColorMaterial(disp, glColorMaterialWrapper);
  SET_ColorPointer(disp, glColorPointerWrapper);
  SET_ColorSubTable(disp, glColorSubTableWrapper);
  SET_ColorTable(disp, glColorTableWrapper);
  SET_ColorTableParameterfv(disp, glColorTableParameterfvWrapper);
  SET_ColorTableParameteriv(disp, glColorTableParameterivWrapper);
  SET_ConvolutionFilter1D(disp, glConvolutionFilter1DWrapper);
  SET_ConvolutionFilter2D(disp, glConvolutionFilter2DWrapper);
  SET_ConvolutionParameterf(disp, glConvolutionParameterfWrapper);
  SET_ConvolutionParameterfv(disp, glConvolutionParameterfvWrapper);
  SET_ConvolutionParameteri(disp, glConvolutionParameteriWrapper);
  SET_ConvolutionParameteriv(disp, glConvolutionParameterivWrapper);
  SET_CopyColorSubTable(disp, glCopyColorSubTableWrapper);
  SET_CopyColorTable(disp, glCopyColorTableWrapper);
  SET_CopyConvolutionFilter1D(disp, glCopyConvolutionFilter1DWrapper);
  SET_CopyConvolutionFilter2D(disp, glCopyConvolutionFilter2DWrapper);
  SET_CopyPixels(disp, glCopyPixelsWrapper);
  SET_CopyTexImage1D(disp, glCopyTexImage1DWrapper);
  SET_CopyTexImage2D(disp, glCopyTexImage2DWrapper);
  SET_CopyTexSubImage1D(disp, glCopyTexSubImage1DWrapper);
  SET_CopyTexSubImage2D(disp, glCopyTexSubImage2DWrapper);
  SET_CopyTexSubImage3D(disp, glCopyTexSubImage3DWrapper);
  SET_CullFace(disp, glCullFaceWrapper);
  SET_DeleteLists(disp, glDeleteListsWrapper);

  SET_DepthFunc(disp, glDepthFuncWrapper);
  SET_DepthMask(disp, glDepthMaskWrapper);
  SET_DepthRange(disp, glDepthRangeWrapper);
  SET_Disable(disp, glDisableWrapper);
  SET_DisableClientState(disp, glDisableClientStateWrapper);
  SET_DrawArrays(disp, glDrawArraysWrapper);
  SET_DrawBuffer(disp, glDrawBufferWrapper);
  SET_DrawElements(disp, glDrawElementsWrapper);
  SET_DrawPixels(disp, glDrawPixelsWrapper);
  SET_DrawRangeElements(disp, glDrawRangeElementsWrapper);
  SET_EdgeFlag(disp, glEdgeFlagWrapper);
  SET_EdgeFlagPointer(disp, glEdgeFlagPointerWrapper);
  SET_EdgeFlagv(disp, glEdgeFlagvWrapper);
  SET_Enable(disp, glEnableWrapper);
  SET_EnableClientState(disp, glEnableClientStateWrapper);
  SET_End(disp, glEndWrapper);
  SET_EndList(disp, glEndListWrapper);
  SET_EvalCoord1d(disp, glEvalCoord1dWrapper);
  SET_EvalCoord1dv(disp, glEvalCoord1dvWrapper);
  SET_EvalCoord1f(disp, glEvalCoord1fWrapper);
  SET_EvalCoord1fv(disp, glEvalCoord1fvWrapper);
  SET_EvalCoord2d(disp, glEvalCoord2dWrapper);
  SET_EvalCoord2dv(disp, glEvalCoord2dvWrapper);
  SET_EvalCoord2f(disp, glEvalCoord2fWrapper);
  SET_EvalCoord2fv(disp, glEvalCoord2fvWrapper);
  SET_EvalMesh1(disp, glEvalMesh1Wrapper);
  SET_EvalMesh2(disp, glEvalMesh2Wrapper);
  SET_EvalPoint1(disp, glEvalPoint1Wrapper);
  SET_EvalPoint2(disp, glEvalPoint2Wrapper);
  SET_FeedbackBuffer(disp, glFeedbackBufferWrapper);
  SET_Finish(disp, glFinishWrapper);
  SET_Flush(disp, glFlushWrapper);
  SET_Fogf(disp, glFogfWrapper);
  SET_Fogfv(disp, glFogfvWrapper);
  SET_Fogi(disp, glFogiWrapper);
  SET_Fogiv(disp, glFogivWrapper);
  SET_FrontFace(disp, glFrontFaceWrapper);
  SET_Frustum(disp, glFrustumWrapper);
  SET_GenLists(disp, glGenListsWrapper);
  SET_GetBooleanv(disp, glGetBooleanvWrapper);
  SET_GetClipPlane(disp, glGetClipPlaneWrapper);
  SET_GetColorTable(disp, glGetColorTableWrapper);
  SET_GetColorTableParameterfv(disp, glGetColorTableParameterfvWrapper);
  SET_GetColorTableParameteriv(disp, glGetColorTableParameterivWrapper);
  SET_GetConvolutionFilter(disp, glGetConvolutionFilterWrapper);
  SET_GetConvolutionParameterfv(disp, glGetConvolutionParameterfvWrapper);
  SET_GetConvolutionParameteriv(disp, glGetConvolutionParameterivWrapper);
  SET_GetDoublev(disp, glGetDoublevWrapper);
  SET_GetError(disp, glGetErrorWrapper);
  SET_GetFloatv(disp, glGetFloatvWrapper);
  SET_GetHistogram(disp, glGetHistogramWrapper);
  SET_GetHistogramParameterfv(disp, glGetHistogramParameterfvWrapper);
  SET_GetHistogramParameteriv(disp, glGetHistogramParameterivWrapper);

  SET_GetLightfv(disp, glGetLightfvWrapper);
  SET_GetLightiv(disp, glGetLightivWrapper);
  SET_GetMapdv(disp, glGetMapdvWrapper);
  SET_GetMapfv(disp, glGetMapfvWrapper);
  SET_GetMapiv(disp, glGetMapivWrapper);
  SET_GetMaterialfv(disp, glGetMaterialfvWrapper);
  SET_GetMaterialiv(disp, glGetMaterialivWrapper);
  SET_GetMinmax(disp, glGetMinmaxWrapper);
  SET_GetMinmaxParameterfv(disp, glGetMinmaxParameterfvWrapper);
  SET_GetMinmaxParameteriv(disp, glGetMinmaxParameterivWrapper);
  SET_GetPixelMapfv(disp, glGetPixelMapfvWrapper);
  SET_GetPixelMapuiv(disp, glGetPixelMapuivWrapper);
  SET_GetPixelMapusv(disp, glGetPixelMapusvWrapper);
  SET_GetPointerv(disp, glGetPointervWrapper);
  SET_GetPolygonStipple(disp, glGetPolygonStippleWrapper);
  SET_GetSeparableFilter(disp, glGetSeparableFilterWrapper);
  SET_GetTexEnvfv(disp, glGetTexEnvfvWrapper);
  SET_GetTexEnviv(disp, glGetTexEnvivWrapper);
  SET_GetTexGendv(disp, glGetTexGendvWrapper);
  SET_GetTexGenfv(disp, glGetTexGenfvWrapper);
  SET_GetTexGeniv(disp, glGetTexGenivWrapper);
  SET_GetTexImage(disp, glGetTexImageWrapper);
  SET_GetTexLevelParameterfv(disp, glGetTexLevelParameterfvWrapper);
  SET_GetTexLevelParameteriv(disp, glGetTexLevelParameterivWrapper);
  SET_GetTexParameterfv(disp, glGetTexParameterfvWrapper);
  SET_GetTexParameteriv(disp, glGetTexParameterivWrapper);
  SET_Hint(disp, glHintWrapper);
  SET_Histogram(disp, glHistogramWrapper);
  SET_IndexMask(disp, glIndexMaskWrapper);
  SET_IndexPointer(disp, glIndexPointerWrapper);
  SET_Indexd(disp, glIndexdWrapper);
  SET_Indexdv(disp, glIndexdvWrapper);
  SET_Indexf(disp, glIndexfWrapper);
  SET_Indexfv(disp, glIndexfvWrapper);
  SET_Indexi(disp, glIndexiWrapper);
  SET_Indexiv(disp, glIndexivWrapper);
  SET_Indexs(disp, glIndexsWrapper);
  SET_Indexsv(disp, glIndexsvWrapper);
  SET_Indexub(disp, glIndexubWrapper);
  SET_Indexubv(disp, glIndexubvWrapper);
  SET_InitNames(disp, glInitNamesWrapper);
  SET_InterleavedArrays(disp, glInterleavedArraysWrapper);
  SET_IsEnabled(disp, glIsEnabledWrapper);
  SET_IsList(disp, glIsListWrapper);
  SET_LightModelf(disp, glLightModelfWrapper);
  SET_LightModelfv(disp, glLightModelfvWrapper);
  SET_LightModeli(disp, glLightModeliWrapper);
  SET_LightModeliv(disp, glLightModelivWrapper);
  SET_Lightf(disp, glLightfWrapper);
  SET_Lightfv(disp, glLightfvWrapper);
  SET_Lighti(disp, glLightiWrapper);
  SET_Lightiv(disp, glLightivWrapper);
  SET_LineStipple(disp, glLineStippleWrapper);
  SET_LineWidth(disp, glLineWidthWrapper);
  SET_ListBase(disp, glListBaseWrapper);
  SET_LoadIdentity(disp, glLoadIdentityWrapper);
  SET_LoadMatrixd(disp, glLoadMatrixdWrapper);
  SET_LoadMatrixf(disp, glLoadMatrixfWrapper);
  SET_LoadName(disp, glLoadNameWrapper);
  SET_LogicOp(disp, glLogicOpWrapper);
  SET_Map1d(disp, glMap1dWrapper);
  SET_Map1f(disp, glMap1fWrapper);
  SET_Map2d(disp, glMap2dWrapper);
  SET_Map2f(disp, glMap2fWrapper);
  SET_MapGrid1d(disp, glMapGrid1dWrapper);
  SET_MapGrid1f(disp, glMapGrid1fWrapper);
  SET_MapGrid2d(disp, glMapGrid2dWrapper);
  SET_MapGrid2f(disp, glMapGrid2fWrapper);
  SET_Materialf(disp, glMaterialfWrapper);
  SET_Materialfv(disp, glMaterialfvWrapper);
  SET_Materiali(disp, glMaterialiWrapper);
  SET_Materialiv(disp, glMaterialivWrapper);
  SET_MatrixMode(disp, glMatrixModeWrapper);
  SET_Minmax(disp, glMinmaxWrapper);
  SET_MultMatrixd(disp, glMultMatrixdWrapper);
  SET_MultMatrixf(disp, glMultMatrixfWrapper);
  SET_NewList(disp, glNewListWrapper);
  SET_Normal3b(disp, glNormal3bWrapper);
  SET_Normal3bv(disp, glNormal3bvWrapper);
  SET_Normal3d(disp, glNormal3dWrapper);
  SET_Normal3dv(disp, glNormal3dvWrapper);
  SET_Normal3f(disp, glNormal3fWrapper);
  SET_Normal3fv(disp, glNormal3fvWrapper);
  SET_Normal3i(disp, glNormal3iWrapper);
  SET_Normal3iv(disp, glNormal3ivWrapper);
  SET_Normal3s(disp, glNormal3sWrapper);
  SET_Normal3sv(disp, glNormal3svWrapper);
  SET_NormalPointer(disp, glNormalPointerWrapper);
  SET_Ortho(disp, glOrthoWrapper);
  SET_PassThrough(disp, glPassThroughWrapper);
  SET_PixelMapfv(disp, glPixelMapfvWrapper);
  SET_PixelMapuiv(disp, glPixelMapuivWrapper);
  SET_PixelMapusv(disp, glPixelMapusvWrapper);
  SET_PixelStoref(disp, glPixelStorefWrapper);
  SET_PixelStorei(disp, glPixelStoreiWrapper);
  SET_PixelTransferf(disp, glPixelTransferfWrapper);
  SET_PixelTransferi(disp, glPixelTransferiWrapper);
  SET_PixelZoom(disp, glPixelZoomWrapper);
  SET_PointSize(disp, glPointSizeWrapper);
  SET_PolygonMode(disp, glPolygonModeWrapper);
  SET_PolygonOffset(disp, glPolygonOffsetWrapper);
  SET_PolygonStipple(disp, glPolygonStippleWrapper);
  SET_PopAttrib(disp, glPopAttribWrapper);
  SET_PopClientAttrib(disp, glPopClientAttribWrapper);
  SET_PopMatrix(disp, glPopMatrixWrapper);
  SET_PopName(disp, glPopNameWrapper);
  SET_PushAttrib(disp, glPushAttribWrapper);
  SET_PushClientAttrib(disp, glPushClientAttribWrapper);
  SET_PushMatrix(disp, glPushMatrixWrapper);
  SET_PushName(disp, glPushNameWrapper);
  SET_RasterPos2d(disp, glRasterPos2dWrapper);
  SET_RasterPos2dv(disp, glRasterPos2dvWrapper);
  SET_RasterPos2f(disp, glRasterPos2fWrapper);
  SET_RasterPos2fv(disp, glRasterPos2fvWrapper);
  SET_RasterPos2i(disp, glRasterPos2iWrapper);
  SET_RasterPos2iv(disp, glRasterPos2ivWrapper);
  SET_RasterPos2s(disp, glRasterPos2sWrapper);
  SET_RasterPos2sv(disp, glRasterPos2svWrapper);
  SET_RasterPos3d(disp, glRasterPos3dWrapper);
  SET_RasterPos3dv(disp, glRasterPos3dvWrapper);
  SET_RasterPos3f(disp, glRasterPos3fWrapper);
  SET_RasterPos3fv(disp, glRasterPos3fvWrapper);
  SET_RasterPos3i(disp, glRasterPos3iWrapper);
  SET_RasterPos3iv(disp, glRasterPos3ivWrapper);
  SET_RasterPos3s(disp, glRasterPos3sWrapper);
  SET_RasterPos3sv(disp, glRasterPos3svWrapper);
  SET_RasterPos4d(disp, glRasterPos4dWrapper);
  SET_RasterPos4dv(disp, glRasterPos4dvWrapper);
  SET_RasterPos4f(disp, glRasterPos4fWrapper);
  SET_RasterPos4fv(disp, glRasterPos4fvWrapper);
  SET_RasterPos4i(disp, glRasterPos4iWrapper);
  SET_RasterPos4iv(disp, glRasterPos4ivWrapper);
  SET_RasterPos4s(disp, glRasterPos4sWrapper);
  SET_RasterPos4sv(disp, glRasterPos4svWrapper);
  SET_ReadBuffer(disp, glReadBufferWrapper);
  SET_ReadPixels(disp, glReadPixelsWrapper);
  SET_Rectd(disp, glRectdWrapper);
  SET_Rectdv(disp, glRectdvWrapper);
  SET_Rectf(disp, glRectfWrapper);
  SET_Rectfv(disp, glRectfvWrapper);
  SET_Recti(disp, glRectiWrapper);
  SET_Rectiv(disp, glRectivWrapper);
  SET_Rects(disp, glRectsWrapper);
  SET_Rectsv(disp, glRectsvWrapper);
  SET_RenderMode(disp, glRenderModeWrapper);
  SET_ResetHistogram(disp, glResetHistogramWrapper);
  SET_ResetMinmax(disp, glResetMinmaxWrapper);
  SET_Rotated(disp, glRotatedWrapper);
  SET_Rotatef(disp, glRotatefWrapper);
  SET_Scaled(disp, glScaledWrapper);
  SET_Scalef(disp, glScalefWrapper);
  SET_Scissor(disp, glScissorWrapper);
  SET_SelectBuffer(disp, glSelectBufferWrapper);
  SET_SeparableFilter2D(disp, glSeparableFilter2DWrapper);
  SET_ShadeModel(disp, glShadeModelWrapper);
  SET_StencilFunc(disp, glStencilFuncWrapper);
  SET_StencilMask(disp, glStencilMaskWrapper);
  SET_StencilOp(disp, glStencilOpWrapper);
  SET_TexCoord1d(disp, glTexCoord1dWrapper);
  SET_TexCoord1dv(disp, glTexCoord1dvWrapper);
  SET_TexCoord1f(disp, glTexCoord1fWrapper);
  SET_TexCoord1fv(disp, glTexCoord1fvWrapper);
  SET_TexCoord1i(disp, glTexCoord1iWrapper);
  SET_TexCoord1iv(disp, glTexCoord1ivWrapper);
  SET_TexCoord1s(disp, glTexCoord1sWrapper);
  SET_TexCoord1sv(disp, glTexCoord1svWrapper);
  SET_TexCoord2d(disp, glTexCoord2dWrapper);
  SET_TexCoord2dv(disp, glTexCoord2dvWrapper);
  SET_TexCoord2f(disp, glTexCoord2fWrapper);
  SET_TexCoord2fv(disp, glTexCoord2fvWrapper);
  SET_TexCoord2i(disp, glTexCoord2iWrapper);
  SET_TexCoord2iv(disp, glTexCoord2ivWrapper);
  SET_TexCoord2s(disp, glTexCoord2sWrapper);
  SET_TexCoord2sv(disp, glTexCoord2svWrapper);
  SET_TexCoord3d(disp, glTexCoord3dWrapper);
  SET_TexCoord3dv(disp, glTexCoord3dvWrapper);
  SET_TexCoord3f(disp, glTexCoord3fWrapper);
  SET_TexCoord3fv(disp, glTexCoord3fvWrapper);
  SET_TexCoord3i(disp, glTexCoord3iWrapper);
  SET_TexCoord3iv(disp, glTexCoord3ivWrapper);
  SET_TexCoord3s(disp, glTexCoord3sWrapper);
  SET_TexCoord3sv(disp, glTexCoord3svWrapper);
  SET_TexCoord4d(disp, glTexCoord4dWrapper);
  SET_TexCoord4dv(disp, glTexCoord4dvWrapper);
  SET_TexCoord4f(disp, glTexCoord4fWrapper);
  SET_TexCoord4fv(disp, glTexCoord4fvWrapper);
  SET_TexCoord4i(disp, glTexCoord4iWrapper);
  SET_TexCoord4iv(disp, glTexCoord4ivWrapper);
  SET_TexCoord4s(disp, glTexCoord4sWrapper);
  SET_TexCoord4sv(disp, glTexCoord4svWrapper);
  SET_TexCoordPointer(disp, glTexCoordPointerWrapper);
  SET_TexEnvf(disp, glTexEnvfWrapper);
  SET_TexEnvfv(disp, glTexEnvfvWrapper);
  SET_TexEnvi(disp, glTexEnviWrapper);
  SET_TexEnviv(disp, glTexEnvivWrapper);
  SET_TexGend(disp, glTexGendWrapper);
  SET_TexGendv(disp, glTexGendvWrapper);
  SET_TexGenf(disp, glTexGenfWrapper);
  SET_TexGenfv(disp, glTexGenfvWrapper);
  SET_TexGeni(disp, glTexGeniWrapper);
  SET_TexGeniv(disp, glTexGenivWrapper);
  SET_TexImage1D(disp, glTexImage1DWrapper);
  SET_TexImage2D(disp, glTexImage2DWrapper);
  SET_TexImage3D(disp, glTexImage3DWrapper);
  SET_TexParameterf(disp, glTexParameterfWrapper);
  SET_TexParameterfv(disp, glTexParameterfvWrapper);
  SET_TexParameteri(disp, glTexParameteriWrapper);
  SET_TexParameteriv(disp, glTexParameterivWrapper);
  SET_TexSubImage1D(disp, glTexSubImage1DWrapper);
  SET_TexSubImage2D(disp, glTexSubImage2DWrapper);
  SET_TexSubImage3D(disp, glTexSubImage3DWrapper);
  SET_Translated(disp, glTranslatedWrapper);
  SET_Translatef(disp, glTranslatefWrapper);
  SET_Vertex2d(disp, glVertex2dWrapper);
  SET_Vertex2dv(disp, glVertex2dvWrapper);
  SET_Vertex2f(disp, glVertex2fWrapper);
  SET_Vertex2fv(disp, glVertex2fvWrapper);
  SET_Vertex2i(disp, glVertex2iWrapper);
  SET_Vertex2iv(disp, glVertex2ivWrapper);
  SET_Vertex2s(disp, glVertex2sWrapper);
  SET_Vertex2sv(disp, glVertex2svWrapper);
  SET_Vertex3d(disp, glVertex3dWrapper);
  SET_Vertex3dv(disp, glVertex3dvWrapper);
  SET_Vertex3f(disp, glVertex3fWrapper);
  SET_Vertex3fv(disp, glVertex3fvWrapper);
  SET_Vertex3i(disp, glVertex3iWrapper);
  SET_Vertex3iv(disp, glVertex3ivWrapper);
  SET_Vertex3s(disp, glVertex3sWrapper);
  SET_Vertex3sv(disp, glVertex3svWrapper);
  SET_Vertex4d(disp, glVertex4dWrapper);
  SET_Vertex4dv(disp, glVertex4dvWrapper);
  SET_Vertex4f(disp, glVertex4fWrapper);
  SET_Vertex4fv(disp, glVertex4fvWrapper);
  SET_Vertex4i(disp, glVertex4iWrapper);
  SET_Vertex4iv(disp, glVertex4ivWrapper);
  SET_Vertex4s(disp, glVertex4sWrapper);
  SET_Vertex4sv(disp, glVertex4svWrapper);
  SET_VertexPointer(disp, glVertexPointerWrapper);
  SET_Viewport(disp, glViewportWrapper);

#if GL_VERSION_2_0
/*   SET_AttachShader(disp, glAttachShaderWrapper); */
/*   SET_DeleteShader(disp, glDeleteShaderWrapper); */
/*   SET_DetachShader(disp, glDetachShaderWrapper); */
/*   SET_GetAttachedShaders(disp, glGetAttachedShadersWrapper); */
/*   SET_GetProgramInfoLog(disp, glGetProgramInfoLogWrapper); */
/*   SET_GetShaderInfoLog(disp, glGetShaderInfoLogWrapper); */
/*   SET_GetShaderiv(disp, glGetShaderivWrapper); */
/*   SET_IsShader(disp, glIsShaderWrapper); */
/*   SET_StencilFuncSeparate(disp, glStencilFuncSeparateWrapper); */
/*   SET_StencilMaskSeparate(disp, glStencilMaskSeparateWrapper); */
/*   SET_StencilOpSeparate(disp, glStencilOpSeparateWrapper); */
#endif

#if GL_VERSION_2_1
/*   SET_UniformMatrix2x3fv(disp, glUniformMatrix2x3fvWrapper); */
/*   SET_UniformMatrix2x4fv(disp, glUniformMatrix2x4fvWrapper); */
/*   SET_UniformMatrix3x2fv(disp, glUniformMatrix3x2fvWrapper); */
/*   SET_UniformMatrix3x4fv(disp, glUniformMatrix3x4fvWrapper); */
/*   SET_UniformMatrix4x2fv(disp, glUniformMatrix4x2fvWrapper); */
/*   SET_UniformMatrix4x3fv(disp, glUniformMatrix4x3fvWrapper); */
#endif

#if GL_APPLE_vertex_array_object
/*   SET_BindVertexArrayAPPLE(disp, glBindVertexArrayAPPLEWrapper); */
/*   SET_DeleteVertexArraysAPPLE(disp, glDeleteVertexArraysAPPLEWrapper); */
/*   SET_GenVertexArraysAPPLE(disp, glGenVertexArraysAPPLEWrapper); */
/*   SET_IsVertexArrayAPPLE(disp, glIsVertexArrayAPPLEWrapper); */
#endif

#if GL_ARB_draw_buffers
/*   SET_DrawBuffersARB(disp, glDrawBuffersARBWrapper); */
#endif

#if GL_ARB_multisample
  SET_SampleCoverageARB(disp, glSampleCoverageWrapper);
#endif

#if GL_ARB_multitexture
  SET_ActiveTextureARB(disp, glActiveTextureWrapper);
  SET_ClientActiveTextureARB(disp, glClientActiveTextureWrapper);
  SET_MultiTexCoord1dARB(disp, glMultiTexCoord1dWrapper);
  SET_MultiTexCoord1dvARB(disp, glMultiTexCoord1dvWrapper);
  SET_MultiTexCoord1fARB(disp, glMultiTexCoord1fWrapper);
  SET_MultiTexCoord1fvARB(disp, glMultiTexCoord1fvWrapper);
  SET_MultiTexCoord1iARB(disp, glMultiTexCoord1iWrapper);
  SET_MultiTexCoord1ivARB(disp, glMultiTexCoord1ivWrapper);
  SET_MultiTexCoord1sARB(disp, glMultiTexCoord1sWrapper);
  SET_MultiTexCoord1svARB(disp, glMultiTexCoord1svWrapper);
  SET_MultiTexCoord2dARB(disp, glMultiTexCoord2dWrapper);
  SET_MultiTexCoord2dvARB(disp, glMultiTexCoord2dvWrapper);
  SET_MultiTexCoord2fARB(disp, glMultiTexCoord2fWrapper);
  SET_MultiTexCoord2fvARB(disp, glMultiTexCoord2fvWrapper);
  SET_MultiTexCoord2iARB(disp, glMultiTexCoord2iWrapper);
  SET_MultiTexCoord2ivARB(disp, glMultiTexCoord2ivWrapper);
  SET_MultiTexCoord2sARB(disp, glMultiTexCoord2sWrapper);
  SET_MultiTexCoord2svARB(disp, glMultiTexCoord2svWrapper);
  SET_MultiTexCoord3dARB(disp, glMultiTexCoord3dWrapper);
  SET_MultiTexCoord3dvARB(disp, glMultiTexCoord3dvWrapper);
  SET_MultiTexCoord3fARB(disp, glMultiTexCoord3fWrapper);
  SET_MultiTexCoord3fvARB(disp, glMultiTexCoord3fvWrapper);
  SET_MultiTexCoord3iARB(disp, glMultiTexCoord3iWrapper);
  SET_MultiTexCoord3ivARB(disp, glMultiTexCoord3ivWrapper);
  SET_MultiTexCoord3sARB(disp, glMultiTexCoord3sWrapper);
  SET_MultiTexCoord3svARB(disp, glMultiTexCoord3svWrapper);
  SET_MultiTexCoord4dARB(disp, glMultiTexCoord4dWrapper);
  SET_MultiTexCoord4dvARB(disp, glMultiTexCoord4dvWrapper);
  SET_MultiTexCoord4fARB(disp, glMultiTexCoord4fWrapper);
  SET_MultiTexCoord4fvARB(disp, glMultiTexCoord4fvWrapper);
  SET_MultiTexCoord4iARB(disp, glMultiTexCoord4iWrapper);
  SET_MultiTexCoord4ivARB(disp, glMultiTexCoord4ivWrapper);
  SET_MultiTexCoord4sARB(disp, glMultiTexCoord4sWrapper);
  SET_MultiTexCoord4svARB(disp, glMultiTexCoord4svWrapper);
#endif

#if GL_ARB_occlusion_query
/*   SET_BeginQueryARB(disp, glBeginQueryARBWrapper); */
/*   SET_DeleteQueriesARB(disp, glDeleteQueriesARBWrapper); */
/*   SET_EndQueryARB(disp, glEndQueryARBWrapper); */
/*   SET_GenQueriesARB(disp, glGenQueriesARBWrapper); */
/*   SET_GetQueryObjectivARB(disp, glGetQueryObjectivARBWrapper); */
/*   SET_GetQueryObjectuivARB(disp, glGetQueryObjectuivARBWrapper); */
/*   SET_GetQueryivARB(disp, glGetQueryivARBWrapper); */
/*   SET_IsQueryARB(disp, glIsQueryARBWrapper); */
#endif

#if GL_ARB_shader_objects
/*   SET_AttachObjectARB(disp, glAttachObjectARBWrapper); */
/*   SET_CompileShaderARB(disp, glCompileShaderARBWrapper); */
/*   SET_DeleteObjectARB(disp, glDeleteObjectARBWrapper); */
/*   SET_GetHandleARB(disp, glGetHandleARBWrapper); */
/*   SET_DetachObjectARB(disp, glDetachObjectARBWrapper); */
/*   SET_CreateProgramObjectARB(disp, glCreateProgramObjectARBWrapper); */
/*   SET_CreateShaderObjectARB(disp, glCreateShaderObjectARBWrapper); */
/*   SET_GetInfoLogARB(disp, glGetInfoLogARBWrapper); */
/*   SET_GetActiveUniformARB(disp, glGetActiveUniformARBWrapper); */
/*   SET_GetAttachedObjectsARB(disp, glGetAttachedObjectsARBWrapper); */
/*   SET_GetObjectParameterfvARB(disp, glGetObjectParameterfvARBWrapper); */
/*   SET_GetObjectParameterivARB(disp, glGetObjectParameterivARBWrapper); */
/*   SET_GetShaderSourceARB(disp, glGetShaderSourceARBWrapper); */
/*   SET_GetUniformLocationARB(disp, glGetUniformLocationARBWrapper); */
/*   SET_GetUniformfvARB(disp, glGetUniformfvARBWrapper); */
/*   SET_GetUniformivARB(disp, glGetUniformivARBWrapper); */
/*   SET_LinkProgramARB(disp, glLinkProgramARBWrapper); */
/*   SET_ShaderSourceARB(disp, glShaderSourceARBWrapper); */
/*   SET_Uniform1fARB(disp, glUniform1fARBWrapper); */
/*   SET_Uniform1fvARB(disp, glUniform1fvARBWrapper); */
/*   SET_Uniform1iARB(disp, glUniform1iARBWrapper); */
/*   SET_Uniform1ivARB(disp, glUniform1ivARBWrapper); */
/*   SET_Uniform2fARB(disp, glUniform2fARBWrapper); */
/*   SET_Uniform2fvARB(disp, glUniform2fvARBWrapper); */
/*   SET_Uniform2iARB(disp, glUniform2iARBWrapper); */
/*   SET_Uniform2ivARB(disp, glUniform2ivARBWrapper); */
/*   SET_Uniform3fARB(disp, glUniform3fARBWrapper); */
/*   SET_Uniform3fvARB(disp, glUniform3fvARBWrapper); */
/*   SET_Uniform3iARB(disp, glUniform3iARBWrapper); */
/*   SET_Uniform3ivARB(disp, glUniform3ivARBWrapper); */
/*   SET_Uniform4fARB(disp, glUniform4fARBWrapper); */
/*   SET_Uniform4fvARB(disp, glUniform4fvARBWrapper); */
/*   SET_Uniform4iARB(disp, glUniform4iARBWrapper); */
/*   SET_Uniform4ivARB(disp, glUniform4ivARBWrapper); */
/*   SET_UniformMatrix2fvARB(disp, glUniformMatrix2fvARBWrapper); */
/*   SET_UniformMatrix3fvARB(disp, glUniformMatrix3fvARBWrapper); */
/*   SET_UniformMatrix4fvARB(disp, glUniformMatrix4fvARBWrapper); */
/*   SET_UseProgramObjectARB(disp, glUseProgramObjectARBWrapper); */
/*   SET_ValidateProgramARB(disp, glValidateProgramARBWrapper); */
#endif

#if GL_ARB_texture_compression
  SET_CompressedTexImage1DARB(disp, glCompressedTexImage1DWrapper);
  SET_CompressedTexImage2DARB(disp, glCompressedTexImage2DWrapper);
  SET_CompressedTexImage3DARB(disp, glCompressedTexImage3DWrapper);
  SET_CompressedTexSubImage1DARB(disp, glCompressedTexSubImage1DWrapper);
  SET_CompressedTexSubImage2DARB(disp, glCompressedTexSubImage2DWrapper);
  SET_CompressedTexSubImage3DARB(disp, glCompressedTexSubImage3DWrapper);
  SET_GetCompressedTexImageARB(disp, glGetCompressedTexImageWrapper);
#endif

#if GL_ARB_transpose_matrix
  SET_LoadTransposeMatrixdARB(disp, glLoadTransposeMatrixdWrapper);
  SET_LoadTransposeMatrixfARB(disp, glLoadTransposeMatrixfWrapper);
  SET_MultTransposeMatrixdARB(disp, glMultTransposeMatrixdWrapper);
  SET_MultTransposeMatrixfARB(disp, glMultTransposeMatrixfWrapper);
#endif

#if GL_ARB_vertex_buffer_object
/*   SET_BindBufferARB(disp, glBindBufferARBWrapper); */
/*   SET_BufferDataARB(disp, glBufferDataARBWrapper); */
/*   SET_BufferSubDataARB(disp, glBufferSubDataARBWrapper); */
/*   SET_DeleteBuffersARB(disp, glDeleteBuffersARBWrapper); */
/*   SET_GenBuffersARB(disp, glGenBuffersARBWrapper); */
/*   SET_GetBufferParameterivARB(disp, glGetBufferParameterivARBWrapper); */
/*   SET_GetBufferPointervARB(disp, glGetBufferPointervARBWrapper); */
/*   SET_GetBufferSubDataARB(disp, glGetBufferSubDataARBWrapper); */
/*   SET_IsBufferARB(disp, glIsBufferARBWrapper); */
/*   SET_MapBufferARB(disp, glMapBufferARBWrapper); */
/*   SET_UnmapBufferARB(disp, glUnmapBufferARBWrapper); */
#endif

#if GL_ARB_vertex_program
/*   SET_DisableVertexAttribArrayARB(disp, glDisableVertexAttribArrayARBWrapper); */
/*   SET_EnableVertexAttribArrayARB(disp, glEnableVertexAttribArrayARBWrapper); */
/*   SET_GetProgramEnvParameterdvARB(disp, glGetProgramEnvParameterdvARBWrapper); */
/*   SET_GetProgramEnvParameterfvARB(disp, glGetProgramEnvParameterfvARBWrapper); */
/*   SET_GetProgramLocalParameterdvARB(disp, glGetProgramLocalParameterdvARBWrapper); */
/*   SET_GetProgramLocalParameterfvARB(disp, glGetProgramLocalParameterfvARBWrapper); */
/*   SET_GetProgramStringARB(disp, glGetProgramStringARBWrapper); */
  SET_GetProgramivARB(disp, glGetProgramivARBWrapper);
/*   SET_GetVertexAttribdvARB(disp, glGetVertexAttribdvARBWrapper); */
/*   SET_GetVertexAttribfvARB(disp, glGetVertexAttribfvARBWrapper); */
/*   SET_GetVertexAttribivARB(disp, glGetVertexAttribivARBWrapper); */
/*   SET_ProgramEnvParameter4dARB(disp, glProgramEnvParameter4dARBWrapper); */
/*   SET_ProgramEnvParameter4dvARB(disp, glProgramEnvParameter4dvARBWrapper); */
/*   SET_ProgramEnvParameter4fARB(disp, glProgramEnvParameter4fARBWrapper); */
/*   SET_ProgramEnvParameter4fvARB(disp, glProgramEnvParameter4fvARBWrapper); */
/*   SET_ProgramLocalParameter4dARB(disp, glProgramLocalParameter4dARBWrapper); */
/*   SET_ProgramLocalParameter4dvARB(disp, glProgramLocalParameter4dvARBWrapper); */
/*   SET_ProgramLocalParameter4fARB(disp, glProgramLocalParameter4fARBWrapper); */
/*   SET_ProgramLocalParameter4fvARB(disp, glProgramLocalParameter4fvARBWrapper); */
/*   SET_ProgramStringARB(disp, glProgramStringARBWrapper); */
/*   SET_VertexAttrib1dARB(disp, glVertexAttrib1dARBWrapper); */
/*   SET_VertexAttrib1dvARB(disp, glVertexAttrib1dvARBWrapper); */
/*   SET_VertexAttrib1fARB(disp, glVertexAttrib1fARBWrapper); */
/*   SET_VertexAttrib1fvARB(disp, glVertexAttrib1fvARBWrapper); */
/*   SET_VertexAttrib1sARB(disp, glVertexAttrib1sARBWrapper); */
/*   SET_VertexAttrib1svARB(disp, glVertexAttrib1svARBWrapper); */
/*   SET_VertexAttrib2dARB(disp, glVertexAttrib2dARBWrapper); */
/*   SET_VertexAttrib2dvARB(disp, glVertexAttrib2dvARBWrapper); */
/*   SET_VertexAttrib2fARB(disp, glVertexAttrib2fARBWrapper); */
/*   SET_VertexAttrib2fvARB(disp, glVertexAttrib2fvARBWrapper); */
/*   SET_VertexAttrib2sARB(disp, glVertexAttrib2sARBWrapper); */
/*   SET_VertexAttrib2svARB(disp, glVertexAttrib2svARBWrapper); */
/*   SET_VertexAttrib3dARB(disp, glVertexAttrib3dARBWrapper); */
/*   SET_VertexAttrib3dvARB(disp, glVertexAttrib3dvARBWrapper); */
/*   SET_VertexAttrib3fARB(disp, glVertexAttrib3fARBWrapper); */
/*   SET_VertexAttrib3fvARB(disp, glVertexAttrib3fvARBWrapper); */
/*   SET_VertexAttrib3sARB(disp, glVertexAttrib3sARBWrapper); */
/*   SET_VertexAttrib3svARB(disp, glVertexAttrib3svARBWrapper); */
/*   SET_VertexAttrib4NbvARB(disp, glVertexAttrib4NbvARBWrapper); */
/*   SET_VertexAttrib4NivARB(disp, glVertexAttrib4NivARBWrapper); */
/*   SET_VertexAttrib4NsvARB(disp, glVertexAttrib4NsvARBWrapper); */
/*   SET_VertexAttrib4NubARB(disp, glVertexAttrib4NubARBWrapper); */
/*   SET_VertexAttrib4NubvARB(disp, glVertexAttrib4NubvARBWrapper); */
/*   SET_VertexAttrib4NuivARB(disp, glVertexAttrib4NuivARBWrapper); */
/*   SET_VertexAttrib4NusvARB(disp, glVertexAttrib4NusvARBWrapper); */
/*   SET_VertexAttrib4bvARB(disp, glVertexAttrib4bvARBWrapper); */
/*   SET_VertexAttrib4dARB(disp, glVertexAttrib4dARBWrapper); */
/*   SET_VertexAttrib4dvARB(disp, glVertexAttrib4dvARBWrapper); */
/*   SET_VertexAttrib4fARB(disp, glVertexAttrib4fARBWrapper); */
/*   SET_VertexAttrib4fvARB(disp, glVertexAttrib4fvARBWrapper); */
/*   SET_VertexAttrib4ivARB(disp, glVertexAttrib4ivARBWrapper); */
/*   SET_VertexAttrib4sARB(disp, glVertexAttrib4sARBWrapper); */
/*   SET_VertexAttrib4svARB(disp, glVertexAttrib4svARBWrapper); */
/*   SET_VertexAttrib4ubvARB(disp, glVertexAttrib4ubvARBWrapper); */
/*   SET_VertexAttrib4uivARB(disp, glVertexAttrib4uivARBWrapper); */
/*   SET_VertexAttrib4usvARB(disp, glVertexAttrib4usvARBWrapper); */
/*   SET_VertexAttribPointerARB(disp, glVertexAttribPointerARBWrapper); */
#endif

#if GL_ARB_vertex_shader
/*   SET_BindAttribLocationARB(disp, glBindAttribLocationARBWrapper); */
/*   SET_GetActiveAttribARB(disp, glGetActiveAttribARBWrapper); */
/*   SET_GetAttribLocationARB(disp, glGetAttribLocationARBWrapper); */
#endif

#if GL_ARB_window_pos
/*   SET_WindowPos2dMESA(disp, glWindowPos2dARBWrapper); */
/*   SET_WindowPos2dvMESA(disp, glWindowPos2dvARBWrapper); */
/*   SET_WindowPos2fMESA(disp, glWindowPos2fARBWrapper); */
/*   SET_WindowPos2fvMESA(disp, glWindowPos2fvARBWrapper); */
/*   SET_WindowPos2iMESA(disp, glWindowPos2iARBWrapper); */
/*   SET_WindowPos2ivMESA(disp, glWindowPos2ivARBWrapper); */
/*   SET_WindowPos2sMESA(disp, glWindowPos2sARBWrapper); */
/*   SET_WindowPos2svMESA(disp, glWindowPos2svARBWrapper); */
/*   SET_WindowPos3dMESA(disp, glWindowPos3dARBWrapper); */
/*   SET_WindowPos3dvMESA(disp, glWindowPos3dvARBWrapper); */
  SET_WindowPos3fMESA(disp, glWindowPos3fARBWrapper);
  SET_WindowPos3fvMESA(disp, glWindowPos3fvARBWrapper);
/*   SET_WindowPos3iMESA(disp, glWindowPos3iARBWrapper); */
/*   SET_WindowPos3ivMESA(disp, glWindowPos3ivARBWrapper); */
/*   SET_WindowPos3sMESA(disp, glWindowPos3sARBWrapper); */
/*   SET_WindowPos3svMESA(disp, glWindowPos3svARBWrapper); */
#endif

#if GL_ATI_fragment_shader
/*   SET_AlphaFragmentOp1ATI(disp, glAlphaFragmentOp1ATIWrapper); */
/*   SET_AlphaFragmentOp2ATI(disp, glAlphaFragmentOp2ATIWrapper); */
/*   SET_AlphaFragmentOp3ATI(disp, glAlphaFragmentOp3ATIWrapper); */
/*   SET_BeginFragmentShaderATI(disp, glBeginFragmentShaderATIWrapper); */
/*   SET_BindFragmentShaderATI(disp, glBindFragmentShaderATIWrapper); */
/*   SET_ColorFragmentOp1ATI(disp, glColorFragmentOp1ATIWrapper); */
/*   SET_ColorFragmentOp2ATI(disp, glColorFragmentOp2ATIWrapper); */
/*   SET_ColorFragmentOp3ATI(disp, glColorFragmentOp3ATIWrapper); */
/*   SET_DeleteFragmentShaderATI(disp, glDeleteFragmentShaderATIWrapper); */
/*   SET_EndFragmentShaderATI(disp, glEndFragmentShaderATIWrapper); */
/*   SET_GenFragmentShadersATI(disp, glGenFragmentShadersATIWrapper); */
/*   SET_PassTexCoordATI(disp, glPassTexCoordATIWrapper); */
/*   SET_SampleMapATI(disp, glSampleMapATIWrapper); */
/*   SET_SetFragmentShaderConstantATI(disp, glSetFragmentShaderConstantATIWrapper); */
#elif GL_EXT_fragment_shader
  SET_AlphaFragmentOp1ATI(disp, glAlphaFragmentOp1EXTWrapper);
  SET_AlphaFragmentOp2ATI(disp, glAlphaFragmentOp2EXTWrapper);
  SET_AlphaFragmentOp3ATI(disp, glAlphaFragmentOp3EXTWrapper);
  SET_BeginFragmentShaderATI(disp, glBeginFragmentShaderEXTWrapper);
  SET_BindFragmentShaderATI(disp, glBindFragmentShaderEXTWrapper);
  SET_ColorFragmentOp1ATI(disp, glColorFragmentOp1EXTWrapper);
  SET_ColorFragmentOp2ATI(disp, glColorFragmentOp2EXTWrapper);
  SET_ColorFragmentOp3ATI(disp, glColorFragmentOp3EXTWrapper);
  SET_DeleteFragmentShaderATI(disp, glDeleteFragmentShaderEXTWrapper);
  SET_EndFragmentShaderATI(disp, glEndFragmentShaderEXTWrapper);
  SET_GenFragmentShadersATI(disp, glGenFragmentShadersEXTWrapper);
  SET_PassTexCoordATI(disp, glPassTexCoordEXTWrapper);
  SET_SampleMapATI(disp, glSampleMapEXTWrapper);
  SET_SetFragmentShaderConstantATI(disp, glSetFragmentShaderConstantEXTWrapper);
#endif

#if GL_ATI_separate_stencil
/*   SET_StencilFuncSeparateATI(disp, glStencilFuncSeparateATIWrapper); */
#endif

#if GL_EXT_blend_equation_separate
/*   SET_BlendEquationSeparateEXT(disp, glBlendEquationSeparateEXTWrapper); */
#endif

#if GL_EXT_blend_func_separate
  SET_BlendFuncSeparateEXT(disp, glBlendFuncSeparateWrapper);
#endif

#if GL_EXT_depth_bounds_test
/*   SET_DepthBoundsEXT(disp, glDepthBoundsEXTWrapper); */
#endif

#if GL_EXT_compiled_vertex_array
/*   SET_LockArraysEXT(disp, glLockArraysEXTWrapper); */
/*   SET_UnlockArraysEXT(disp, glUnlockArraysEXTWrapper); */
#endif

#if GL_EXT_cull_vertex
/*   SET_CullParameterdvEXT(disp, glCullParameterdvEXTWrapper); */
/*   SET_CullParameterfvEXT(disp, glCullParameterfvEXTWrapper); */
#endif

#if GL_EXT_fog_coord
  SET_FogCoordPointerEXT(disp, glFogCoordPointerWrapper);
/*   SET_FogCoorddEXT(disp, glFogCoorddWrapper); */
  SET_FogCoorddvEXT(disp, glFogCoorddvWrapper);
/*   SET_FogCoordfEXT(disp, glFogCoordfWrapper); */
  SET_FogCoordfvEXT(disp, glFogCoordfvWrapper);
#endif

#if GL_EXT_framebuffer_blit
/*   SET_BlitFramebufferEXT(disp, glBlitFramebufferEXTWrapper); */
#endif

#if GL_EXT_framebuffer_object
/*   SET_BindFramebufferEXT(disp, glBindFramebufferEXTWrapper); */
/*   SET_BindRenderbufferEXT(disp, glBindRenderbufferEXTWrapper); */
/*   SET_CheckFramebufferStatusEXT(disp, glCheckFramebufferStatusEXTWrapper); */
/*   SET_DeleteFramebuffersEXT(disp, glDeleteFramebuffersEXTWrapper); */
/*   SET_DeleteRenderbuffersEXT(disp, glDeleteRenderbuffersEXTWrapper); */
/*   SET_FramebufferRenderbufferEXT(disp, glFramebufferRenderbufferEXTWrapper); */
/*   SET_FramebufferTexture1DEXT(disp, glFramebufferTexture1DEXTWrapper); */
/*   SET_FramebufferTexture2DEXT(disp, glFramebufferTexture2DEXTWrapper); */
/*   SET_FramebufferTexture3DEXT(disp, glFramebufferTexture3DEXTWrapper); */
/*   SET_GenerateMipmapEXT(disp, glGenerateMipmapEXTWrapper); */
/*   SET_GenFramebuffersEXT(disp, glGenFramebuffersEXTWrapper); */
/*   SET_GenRenderbuffersEXT(disp, glGenRenderbuffersEXTWrapper); */
/*   SET_GetFramebufferAttachmentParameterivEXT(disp, glGetFramebufferAttachmentParameterivEXTWrapper); */
/*   SET_GetRenderbufferParameterivEXT(disp, glGetRenderbufferParameterivEXTWrapper); */
/*   SET_IsFramebufferEXT(disp, glIsFramebufferEXTWrapper); */
/*   SET_IsRenderbufferEXT(disp, glIsRenderbufferEXTWrapper); */
/*   SET_RenderbufferStorageEXT(disp, glRenderbufferStorageEXTWrapper); */
#endif

#if GL_EXT_gpu_program_parameters
/*   SET_ProgramEnvParameters4fvEXT(disp, glProgramEnvParameters4fvEXTWrapper); */
/*   SET_ProgramLocalParameters4fvEXT(disp, glProgramLocalParameters4fvEXTWrapper); */
#endif

#if GL_EXT_multi_draw_arrays
/*   SET_MultiDrawArraysEXT(disp, glMultiDrawArraysEXTWrapper); */
/*   SET_MultiDrawElementsEXT(disp, glMultiDrawElementsEXTWrapper); */
#endif

#if GL_EXT_point_parameters
/*   SET_PointParameterfEXT(disp, glPointParameterfEXTWrapper); */
/*   SET_PointParameterfvEXT(disp, glPointParameterfvEXTWrapper); */
#elif GL_ARB_point_parameters
  SET_PointParameterfEXT(disp, glPointParameterfARBWrapper);
  SET_PointParameterfvEXT(disp, glPointParameterfvARBWrapper);
#endif

#if GL_EXT_polygon_offset
/*   SET_PolygonOffsetEXT(disp, glPolygonOffsetEXTWrapper); */
#endif

#if GL_EXT_secondary_color
/*   SET_SecondaryColor3bEXT(disp, glSecondaryColor3bWrapper); */
  SET_SecondaryColor3bvEXT(disp, glSecondaryColor3bvWrapper);
/*   SET_SecondaryColor3dEXT(disp, glSecondaryColor3dWrapper); */
  SET_SecondaryColor3dvEXT(disp, glSecondaryColor3dvWrapper);
/*   SET_SecondaryColor3fEXT(disp, glSecondaryColor3fWrapper); */
  SET_SecondaryColor3fvEXT(disp, glSecondaryColor3fvWrapper);
/*   SET_SecondaryColor3iEXT(disp, glSecondaryColor3iWrapper); */
  SET_SecondaryColor3ivEXT(disp, glSecondaryColor3ivWrapper);
/*   SET_SecondaryColor3sEXT(disp, glSecondaryColor3sWrapper); */
  SET_SecondaryColor3svEXT(disp, glSecondaryColor3svWrapper);
/*   SET_SecondaryColor3ubEXT(disp, glSecondaryColor3ubWrapper); */
  SET_SecondaryColor3ubvEXT(disp, glSecondaryColor3ubvWrapper);
/*   SET_SecondaryColor3uiEXT(disp, glSecondaryColor3uiWrapper); */
  SET_SecondaryColor3uivEXT(disp, glSecondaryColor3uivWrapper);
/*   SET_SecondaryColor3usEXT(disp, glSecondaryColor3usWrapper); */
  SET_SecondaryColor3usvEXT(disp, glSecondaryColor3usvWrapper);
  SET_SecondaryColorPointerEXT(disp, glSecondaryColorPointerWrapper);
#endif

#if GL_EXT_stencil_two_side
  SET_ActiveStencilFaceEXT(disp, glActiveStencilFaceEXTWrapper);
#endif

#if GL_EXT_timer_query
/*   SET_GetQueryObjecti64vEXT(disp, glGetQueryObjecti64vEXTWrapper); */
/*   SET_GetQueryObjectui64vEXT(disp, glGetQueryObjectui64vEXTWrapper); */
#endif

#if GL_EXT_vertex_array
/*   SET_ColorPointerEXT(disp, glColorPointerEXTWrapper); */
/*   SET_EdgeFlagPointerEXT(disp, glEdgeFlagPointerEXTWrapper); */
/*   SET_IndexPointerEXT(disp, glIndexPointerEXTWrapper); */
/*   SET_NormalPointerEXT(disp, glNormalPointerEXTWrapper); */
/*   SET_TexCoordPointerEXT(disp, glTexCoordPointerEXTWrapper); */
/*   SET_VertexPointerEXT(disp, glVertexPointerEXTWrapper); */
#endif

#if GL_IBM_multimode_draw_arrays
/*   SET_MultiModeDrawArraysIBM(disp, glMultiModeDrawArraysIBMWrapper); */
/*   SET_MultiModeDrawElementsIBM(disp, glMultiModeDrawElementsIBMWrapper); */
#endif

#if GL_MESA_resize_buffers
/*   SET_ResizeBuffersMESA(disp, glResizeBuffersMESAWrapper); */
#endif

#if GL_MESA_window_pos
/*   SET_WindowPos4dMESA(disp, glWindowPos4dMESAWrapper); */
/*   SET_WindowPos4dvMESA(disp, glWindowPos4dvMESAWrapper); */
/*   SET_WindowPos4fMESA(disp, glWindowPos4fMESAWrapper); */
/*   SET_WindowPos4fvMESA(disp, glWindowPos4fvMESAWrapper); */
/*   SET_WindowPos4iMESA(disp, glWindowPos4iMESAWrapper); */
/*   SET_WindowPos4ivMESA(disp, glWindowPos4ivMESAWrapper); */
/*   SET_WindowPos4sMESA(disp, glWindowPos4sMESAWrapper); */
/*   SET_WindowPos4svMESA(disp, glWindowPos4svMESAWrapper); */
#endif

#if GL_NV_fence
/*   SET_DeleteFencesNV(disp, glDeleteFencesNVWrapper); */
/*   SET_FinishFenceNV(disp, glFinishFenceNVWrapper); */
/*   SET_GenFencesNV(disp, glGenFencesNVWrapper); */
/*   SET_GetFenceivNV(disp, glGetFenceivNVWrapper); */
/*   SET_IsFenceNV(disp, glIsFenceNVWrapper); */
/*   SET_SetFenceNV(disp, glSetFenceNVWrapper); */
/*   SET_TestFenceNV(disp, glTestFenceNVWrapper); */
#endif

#if GL_NV_fragment_program
/*   SET_GetProgramNamedParameterdvNV(disp, glGetProgramNamedParameterdvNVWrapper); */
/*   SET_GetProgramNamedParameterfvNV(disp, glGetProgramNamedParameterfvNVWrapper); */
/*   SET_ProgramNamedParameter4dNV(disp, glProgramNamedParameter4dNVWrapper); */
/*   SET_ProgramNamedParameter4dvNV(disp, glProgramNamedParameter4dvNVWrapper); */
/*   SET_ProgramNamedParameter4fNV(disp, glProgramNamedParameter4fNVWrapper); */
/*   SET_ProgramNamedParameter4fvNV(disp, glProgramNamedParameter4fvNVWrapper); */
#endif

#if GL_NV_geometry_program4
/*   SET_FramebufferTextureLayerEXT(disp, glFramebufferTextureLayerEXTWrapper); */
#endif

#if GL_NV_point_sprite
  SET_PointParameteriNV(disp, glPointParameteriNVWrapper);
  SET_PointParameterivNV(disp, glPointParameterivNVWrapper);
#endif

#if GL_NV_register_combiners
/*   SET_CombinerInputNV(disp, glCombinerInputNVWrapper); */
/*   SET_CombinerOutputNV(disp, glCombinerOutputNVWrapper); */
/*   SET_CombinerParameterfNV(disp, glCombinerParameterfNVWrapper); */
/*   SET_CombinerParameterfvNV(disp, glCombinerParameterfvNVWrapper); */
/*   SET_CombinerParameteriNV(disp, glCombinerParameteriNVWrapper); */
/*   SET_CombinerParameterivNV(disp, glCombinerParameterivNVWrapper); */
/*   SET_FinalCombinerInputNV(disp, glFinalCombinerInputNVWrapper); */
/*   SET_GetCombinerInputParameterfvNV(disp, glGetCombinerInputParameterfvNVWrapper); */
/*   SET_GetCombinerInputParameterivNV(disp, glGetCombinerInputParameterivNVWrapper); */
/*   SET_GetCombinerOutputParameterfvNV(disp, glGetCombinerOutputParameterfvNVWrapper); */
/*   SET_GetCombinerOutputParameterivNV(disp, glGetCombinerOutputParameterivNVWrapper); */
/*   SET_GetFinalCombinerInputParameterfvNV(disp, glGetFinalCombinerInputParameterfvNVWrapper); */
/*   SET_GetFinalCombinerInputParameterivNV(disp, glGetFinalCombinerInputParameterivNVWrapper); */
#endif

#if GL_NV_vertex_array_range
/*   SET_FlushVertexArrayRangeNV(disp, glFlushVertexArrayRangeNVWrapper); */
/*   SET_VertexArrayRangeNV(disp, glVertexArrayRangeNVWrapper); */
#endif

#if GL_NV_vertex_program
/*   SET_AreProgramsResidentNV(disp, glAreProgramsResidentNVWrapper); */
/*   SET_BindProgramNV(disp, glBindProgramNVWrapper); */
/*   SET_DeleteProgramsNV(disp, glDeleteProgramsNVWrapper); */
/*   SET_ExecuteProgramNV(disp, glExecuteProgramNVWrapper); */
/*   SET_GenProgramsNV(disp, glGenProgramsNVWrapper); */
/*   SET_GetProgramParameterdvNV(disp, glGetProgramParameterdvNVWrapper); */
/*   SET_GetProgramParameterfvNV(disp, glGetProgramParameterfvNVWrapper); */
/*   SET_GetProgramStringNV(disp, glGetProgramStringNVWrapper); */
/*   SET_GetProgramivNV(disp, glGetProgramivNVWrapper); */
/*   SET_GetTrackMatrixivNV(disp, glGetTrackMatrixivNVWrapper); */
/*   SET_GetVertexAttribPointervNV(disp, glGetVertexAttribPointervNVWrapper); */
/*   SET_GetVertexAttribdvNV(disp, glGetVertexAttribdvNVWrapper); */
/*   SET_GetVertexAttribfvNV(disp, glGetVertexAttribfvNVWrapper); */
/*   SET_GetVertexAttribivNV(disp, glGetVertexAttribivNVWrapper); */
/*   SET_IsProgramNV(disp, glIsProgramNVWrapper); */
/*   SET_LoadProgramNV(disp, glLoadProgramNVWrapper); */
/*   SET_ProgramParameters4dvNV(disp, glProgramParameters4dvNVWrapper); */
/*   SET_ProgramParameters4fvNV(disp, glProgramParameters4fvNVWrapper); */
/*   SET_RequestResidentProgramsNV(disp, glRequestResidentProgramsNVWrapper); */
/*   SET_TrackMatrixNV(disp, glTrackMatrixNVWrapper); */
/*   SET_VertexAttrib1dNV(disp, glVertexAttrib1dNV) */
/*     SET_VertexAttrib1dvNV(disp, glVertexAttrib1dvNV) */
/*     SET_VertexAttrib1fNV(disp, glVertexAttrib1fNV) */
/*     SET_VertexAttrib1fvNV(disp, glVertexAttrib1fvNV) */
/*     SET_VertexAttrib1sNV(disp, glVertexAttrib1sNV) */
/*     SET_VertexAttrib1svNV(disp, glVertexAttrib1svNV) */
/*     SET_VertexAttrib2dNV(disp, glVertexAttrib2dNV) */
/*     SET_VertexAttrib2dvNV(disp, glVertexAttrib2dvNV) */
/*     SET_VertexAttrib2fNV(disp, glVertexAttrib2fNV) */
/*     SET_VertexAttrib2fvNV(disp, glVertexAttrib2fvNV) */
/*     SET_VertexAttrib2sNV(disp, glVertexAttrib2sNV) */
/*     SET_VertexAttrib2svNV(disp, glVertexAttrib2svNV) */
/*     SET_VertexAttrib3dNV(disp, glVertexAttrib3dNV) */
/*     SET_VertexAttrib3dvNV(disp, glVertexAttrib3dvNV) */
/*     SET_VertexAttrib3fNV(disp, glVertexAttrib3fNV) */
/*     SET_VertexAttrib3fvNV(disp, glVertexAttrib3fvNV) */
/*     SET_VertexAttrib3sNV(disp, glVertexAttrib3sNV) */
/*     SET_VertexAttrib3svNV(disp, glVertexAttrib3svNV) */
/*     SET_VertexAttrib4dNV(disp, glVertexAttrib4dNV) */
/*     SET_VertexAttrib4dvNV(disp, glVertexAttrib4dvNV) */
/*     SET_VertexAttrib4fNV(disp, glVertexAttrib4fNV) */
/*     SET_VertexAttrib4fvNV(disp, glVertexAttrib4fvNV) */
/*     SET_VertexAttrib4sNV(disp, glVertexAttrib4sNV) */
/*     SET_VertexAttrib4svNV(disp, glVertexAttrib4svNV) */
/*     SET_VertexAttrib4ubNV(disp, glVertexAttrib4ubNV) */
/*     SET_VertexAttrib4ubvNV(disp, glVertexAttrib4ubvNV) */
/*     SET_VertexAttribPointerNV(disp, glVertexAttribPointerNV) */
/*     SET_VertexAttribs1dvNV(disp, glVertexAttribs1dvNV) */
/*     SET_VertexAttribs1fvNV(disp, glVertexAttribs1fvNV) */
/*     SET_VertexAttribs1svNV(disp, glVertexAttribs1svNV) */
/*     SET_VertexAttribs2dvNV(disp, glVertexAttribs2dvNV) */
/*     SET_VertexAttribs2fvNV(disp, glVertexAttribs2fvNV) */
/*     SET_VertexAttribs2svNV(disp, glVertexAttribs2svNV) */
/*     SET_VertexAttribs3dvNV(disp, glVertexAttribs3dvNV) */
/*     SET_VertexAttribs3fvNV(disp, glVertexAttribs3fvNV) */
/*     SET_VertexAttribs3svNV(disp, glVertexAttribs3svNV) */
/*     SET_VertexAttribs4dvNV(disp, glVertexAttribs4dvNV) */
/*     SET_VertexAttribs4fvNV(disp, glVertexAttribs4fvNV) */
/*     SET_VertexAttribs4svNV(disp, glVertexAttribs4svNV) */
/*     SET_VertexAttribs4ubvNV(disp, glVertexAttribs4ubvNV) */
#endif

#if GL_SGIS_multisample
  /*
     Pointer Incompatability:
  */
  SET_SampleMaskSGIS(disp, glSampleMaskSGISWrapper);
  SET_SamplePatternSGIS(disp, glSamplePatternSGISWrapper);
#endif

#if GL_SGIS_pixel_texture
/*   SET_GetPixelTexGenParameterfvSGIS(disp, glGetPixelTexGenParameterfvSGISWrapper); */
/*   SET_GetPixelTexGenParameterivSGIS(disp, glGetPixelTexGenParameterivSGISWrapper); */
/*   SET_PixelTexGenParameterfSGIS(disp, glPixelTexGenParameterfSGISWrapper); */
/*   SET_PixelTexGenParameterfvSGIS(disp, glPixelTexGenParameterfvSGISWrapper); */
/*   SET_PixelTexGenParameteriSGIS(disp, glPixelTexGenParameteriSGISWrapper); */
/*   SET_PixelTexGenParameterivSGIS(disp, glPixelTexGenParameterivSGISWrapper); */
/*   SET_PixelTexGenSGIX(disp, glPixelTexGenSGIXWrapper); */
#endif
}

