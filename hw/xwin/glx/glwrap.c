/*
 * GLX implementation that uses Win32's OpenGL
 * Wrapper functions for Win32's OpenGL
 *
 * Authors: Alexander Gottwald
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glxserver.h>
#include <glxext.h>
#include <glapi.h>
#include <dispatch.h>

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
    }
#define RESOLVE(procname, symbol) RESOLVE_RET(procname, symbol,)

/*
 * GL_ARB_imaging
 */

GLAPI void GLAPIENTRY glColorTable( GLenum target, GLenum internalformat,
                                    GLsizei width, GLenum format,
                                    GLenum type, const GLvoid *table )
{
    RESOLVE(PFNGLCOLORTABLEPROC, "glColorTable");
    proc(target, internalformat, width, format, type, table);
}

GLAPI void GLAPIENTRY glColorSubTable( GLenum target,
                                       GLsizei start, GLsizei count,
                                       GLenum format, GLenum type,
                                       const GLvoid *data )
{
    RESOLVE(PFNGLCOLORSUBTABLEPROC, "glColorSubTable");
    proc(target, start, count, format, type, data);
}

GLAPI void GLAPIENTRY glColorTableParameteriv(GLenum target, GLenum pname,
                                              const GLint *params)
{
    RESOLVE(PFNGLCOLORTABLEPARAMETERIVPROC, "glColorTableParameteriv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glColorTableParameterfv(GLenum target, GLenum pname,
                                              const GLfloat *params)
{
    RESOLVE(PFNGLCOLORTABLEPARAMETERFVPROC, "glColorTableParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glCopyColorSubTable( GLenum target, GLsizei start,
                                           GLint x, GLint y, GLsizei width )
{
    RESOLVE(PFNGLCOPYCOLORSUBTABLEPROC, "glCopyColorSubTable");
    proc(target, start, x, y, width);
}

GLAPI void GLAPIENTRY glCopyColorTable( GLenum target, GLenum internalformat,
                                        GLint x, GLint y, GLsizei width )
{
    RESOLVE(PFNGLCOPYCOLORTABLEPROC, "glCopyColorTable");
    proc(target, internalformat, x, y, width);
}


GLAPI void GLAPIENTRY glGetColorTable( GLenum target, GLenum format,
                                       GLenum type, GLvoid *table )
{
    RESOLVE(PFNGLGETCOLORTABLEPROC, "glGetColorTable");
    proc(target, format, type, table);
}

GLAPI void GLAPIENTRY glGetColorTableParameterfv( GLenum target, GLenum pname,
                                                  GLfloat *params )
{
    RESOLVE(PFNGLGETCOLORTABLEPARAMETERFVPROC, "glGetColorTableParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glGetColorTableParameteriv( GLenum target, GLenum pname,
                                                  GLint *params )
{
    RESOLVE(PFNGLGETCOLORTABLEPARAMETERIVPROC, "glGetColorTableParameteriv");
    proc(target, pname, params);
}

#if 0
GLAPI void GLAPIENTRY glBlendEquation( GLenum mode )
{
    RESOLVE(PFNGLBLENDEQUATIONPROC, "glBlendEquation");
    proc(mode);
}

GLAPI void GLAPIENTRY glBlendColor( GLclampf red, GLclampf green,
                                    GLclampf blue, GLclampf alpha )
{
    RESOLVE(PFNGLBLENDCOLORPROC, "glBlendColor");
    proc(red, green, blue, alpha);
}
#endif

GLAPI void GLAPIENTRY glHistogram( GLenum target, GLsizei width,
				   GLenum internalformat, GLboolean sink )
{
    RESOLVE(PFNGLHISTOGRAMPROC, "glHistogram");
    proc(target, width, internalformat, sink);
}

GLAPI void GLAPIENTRY glResetHistogram( GLenum target )
{
    RESOLVE(PFNGLRESETHISTOGRAMPROC, "glResetHistogram");
    proc(target);
}

GLAPI void GLAPIENTRY glGetHistogram( GLenum target, GLboolean reset,
				      GLenum format, GLenum type,
				      GLvoid *values )
{
    RESOLVE(PFNGLGETHISTOGRAMPROC, "glGetHistogram");
    proc(target, reset, format, type, values);
};

GLAPI void GLAPIENTRY glGetHistogramParameterfv( GLenum target, GLenum pname,
						 GLfloat *params )
{
    RESOLVE(PFNGLGETHISTOGRAMPARAMETERFVPROC, "glGetHistogramParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glGetHistogramParameteriv( GLenum target, GLenum pname,
						 GLint *params )
{
    RESOLVE(PFNGLGETHISTOGRAMPARAMETERIVPROC, "glGetHistogramParameteriv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glMinmax( GLenum target, GLenum internalformat,
				GLboolean sink )
{
    RESOLVE(PFNGLMINMAXPROC, "glMinmax");
    proc(target, internalformat, sink);
}

GLAPI void GLAPIENTRY glResetMinmax( GLenum target )
{
    RESOLVE(PFNGLRESETMINMAXPROC, "glResetMinmax");
    proc(target);
}

GLAPI void GLAPIENTRY glGetMinmax( GLenum target, GLboolean reset,
                                   GLenum format, GLenum types,
                                   GLvoid *values )
{
    RESOLVE(PFNGLGETMINMAXPROC, "glGetMinmax");
    proc(target, reset, format, types, values);
}

GLAPI void GLAPIENTRY glGetMinmaxParameterfv( GLenum target, GLenum pname,
					      GLfloat *params )
{
    RESOLVE(PFNGLGETMINMAXPARAMETERFVPROC, "glGetMinmaxParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glGetMinmaxParameteriv( GLenum target, GLenum pname,
					      GLint *params )
{
    RESOLVE(PFNGLGETMINMAXPARAMETERIVPROC, "glGetMinmaxParameteriv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glConvolutionFilter1D( GLenum target,
	GLenum internalformat, GLsizei width, GLenum format, GLenum type,
	const GLvoid *image )
{
    RESOLVE(PFNGLCONVOLUTIONFILTER1DPROC, "glConvolutionFilter1D");
    proc(target, internalformat, width, format, type, image);
}

GLAPI void GLAPIENTRY glConvolutionFilter2D( GLenum target,
	GLenum internalformat, GLsizei width, GLsizei height, GLenum format,
	GLenum type, const GLvoid *image )
{
    RESOLVE(PFNGLCONVOLUTIONFILTER2DPROC, "glConvolutionFilter2D");
    proc(target, internalformat, width, height, format, type, image);
}

GLAPI void GLAPIENTRY glConvolutionParameterf( GLenum target, GLenum pname,
	GLfloat params )
{
    RESOLVE(PFNGLCONVOLUTIONPARAMETERFPROC, "glConvolutionParameterf");
    proc(target, pname, params); 
}

GLAPI void GLAPIENTRY glConvolutionParameterfv( GLenum target, GLenum pname,
	const GLfloat *params )
{
    RESOLVE(PFNGLCONVOLUTIONPARAMETERFVPROC, "glConvolutionParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glConvolutionParameteri( GLenum target, GLenum pname,
	GLint params )
{
    RESOLVE(PFNGLCONVOLUTIONPARAMETERIPROC, "glConvolutionParameteri");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glConvolutionParameteriv( GLenum target, GLenum pname,
	const GLint *params )
{
    RESOLVE(PFNGLCONVOLUTIONPARAMETERIVPROC, "glConvolutionParameteriv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glCopyConvolutionFilter1D( GLenum target,
	GLenum internalformat, GLint x, GLint y, GLsizei width )
{
    RESOLVE(PFNGLCOPYCONVOLUTIONFILTER1DPROC, "glCopyConvolutionFilter1D");
    proc(target, internalformat, x, y, width);
}

GLAPI void GLAPIENTRY glCopyConvolutionFilter2D( GLenum target,
	GLenum internalformat, GLint x, GLint y, GLsizei width,
	GLsizei height)
{
    RESOLVE(PFNGLCOPYCONVOLUTIONFILTER2DPROC, "glCopyConvolutionFilter2D");
    proc(target, internalformat, x, y, width, height);
}

GLAPI void GLAPIENTRY glGetConvolutionFilter( GLenum target, GLenum format,
	GLenum type, GLvoid *image )
{
    RESOLVE(PFNGLGETCONVOLUTIONFILTERPROC, "glGetConvolutionFilter");
    proc(target, format, type, image);
}

GLAPI void GLAPIENTRY glGetConvolutionParameterfv( GLenum target, GLenum pname,
	GLfloat *params )
{
    RESOLVE(PFNGLGETCONVOLUTIONPARAMETERFVPROC, "glGetConvolutionParameterfv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glGetConvolutionParameteriv( GLenum target, GLenum pname,
	GLint *params )
{
    RESOLVE(PFNGLGETCONVOLUTIONPARAMETERIVPROC, "glGetConvolutionParameteriv");
    proc(target, pname, params);
}

GLAPI void GLAPIENTRY glSeparableFilter2D( GLenum target,
	GLenum internalformat, GLsizei width, GLsizei height, GLenum format,
	GLenum type, const GLvoid *row, const GLvoid *column )
{
    RESOLVE(PFNGLSEPARABLEFILTER2DPROC, "glSeparableFilter2D");
    proc(target, internalformat, width, height, format, type, row, column);
}

GLAPI void GLAPIENTRY glGetSeparableFilter( GLenum target, GLenum format,
	GLenum type, GLvoid *row, GLvoid *column, GLvoid *span )
{
    RESOLVE(PFNGLGETSEPARABLEFILTERPROC, "glGetSeparableFilter");
    proc(target, format, type, row, column, span);
}

/*
 * OpenGL 1.2
 */

GLAPI void GLAPIENTRY glTexImage3D( GLenum target, GLint level,
                                      GLint internalFormat,
                                      GLsizei width, GLsizei height,
                                      GLsizei depth, GLint border,
                                      GLenum format, GLenum type,
                                      const GLvoid *pixels )
{
    RESOLVE(PFNGLTEXIMAGE3DPROC, "glTexImage3D");
    proc(target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

GLAPI void GLAPIENTRY glTexSubImage3D( GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLint zoffset, GLsizei width,
                                         GLsizei height, GLsizei depth,
                                         GLenum format,
                                         GLenum type, const GLvoid *pixels)
{
    RESOLVE(PFNGLTEXSUBIMAGE3DPROC, "glTexSubImage3D");
    proc(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

GLAPI void GLAPIENTRY glCopyTexSubImage3D( GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLint zoffset, GLint x,
                                             GLint y, GLsizei width,
                                             GLsizei height )
{
    RESOLVE(PFNGLCOPYTEXSUBIMAGE3DPROC, "glCopyTexSubImage3D");
    proc(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

#if 0
// these are problematic as we need stdcall linakge, can we wrapperise?

/*
 * 20. GL_EXT_texture_object
 */
GLAPI void GLAPIENTRY glGenTexturesEXT( GLsizei n, GLuint *textures )
{
    glGenTextures(n, textures);
}

GLAPI void GLAPIENTRY glDeleteTexturesEXT( GLsizei n, const GLuint *textures)
{
    glDeleteTextures(n, textures);
}

GLAPI void GLAPIENTRY glBindTextureEXT( GLenum target, GLuint texture )
{
    glBindTexture(target, target);
}

GLAPI void GLAPIENTRY glPrioritizeTexturesEXT( GLsizei n, const GLuint *textures, const GLclampf *priorities )
{
    glPrioritizeTextures(n, textures, priorities);
}

GLAPI GLboolean GLAPIENTRY glAreTexturesResidentEXT( GLsizei n, const GLuint *textures, GLboolean *residences )
{
    return glAreTexturesResident(n, textures, residences);
}

GLAPI GLboolean GLAPIENTRY glIsTextureEXT( GLuint texture )
{
    return glIsTexture(texture); 
}
#endif

/*
 * GL_ARB_multitexture (ARB extension 1 and OpenGL 1.2.1)
 */

GLAPI void GLAPIENTRY glActiveTextureARB(GLenum texture)
{
    RESOLVE(PFNGLACTIVETEXTUREARBPROC, "glActiveTextureARB");
    proc(texture);
}

GLAPI void GLAPIENTRY glMultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1DVARBPROC, "glMultiTexCoord1dvARB");
    proc(target, v);
}

GLAPI void GLAPIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1FVARBPROC, "glMultiTexCoord1fvARB");
    proc(target, v);
}

GLAPI void GLAPIENTRY glMultiTexCoord1ivARB(GLenum target, const GLint *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1IVARBPROC, "glMultiTexCoord1ivARB");
    proc(target, v);
}

GLAPI void GLAPIENTRY glMultiTexCoord1svARB(GLenum target, const GLshort *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1SVARBPROC, "glMultiTexCoord1svARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
    RESOLVE(PFNGLMULTITEXCOORD2DVARBPROC, "glMultiTexCoord2dvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
    RESOLVE(PFNGLMULTITEXCOORD2FVARBPROC, "glMultiTexCoord2fvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v)
{
    RESOLVE(PFNGLMULTITEXCOORD2IVARBPROC, "glMultiTexCoord2ivARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord2svARB(GLenum target, const GLshort *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1SVARBPROC, "glMultiTexCoord1svARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
    RESOLVE(PFNGLMULTITEXCOORD3DVARBPROC, "glMultiTexCoord3dvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
    RESOLVE(PFNGLMULTITEXCOORD3FVARBPROC, "glMultiTexCoord3fvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord3ivARB(GLenum target, const GLint *v)
{
    RESOLVE(PFNGLMULTITEXCOORD3IVARBPROC, "glMultiTexCoord3ivARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord3svARB(GLenum target, const GLshort *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1SVARBPROC, "glMultiTexCoord1svARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
    RESOLVE(PFNGLMULTITEXCOORD4DVARBPROC, "glMultiTexCoord4dvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
    RESOLVE(PFNGLMULTITEXCOORD4FVARBPROC, "glMultiTexCoord4fvARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord4ivARB(GLenum target, const GLint *v)
{
    RESOLVE(PFNGLMULTITEXCOORD4IVARBPROC, "glMultiTexCoord4ivARB");
    proc(target, v);
}
GLAPI void GLAPIENTRY glMultiTexCoord4svARB(GLenum target, const GLshort *v)
{
    RESOLVE(PFNGLMULTITEXCOORD1SVARBPROC, "glMultiTexCoord1svARB");
    proc(target, v);
}


GLAPI void GLAPIENTRY glActiveStencilFaceEXT(GLenum face)
{
    RESOLVE(PFNGLACTIVESTENCILFACEEXTPROC, "glActiveStencilFaceEXT");
    proc(face);
}

GLAPI void APIENTRY glPointParameterfARB(GLenum pname, GLfloat param)
{
    RESOLVE(PFNGLPOINTPARAMETERFARBPROC, "glPointParameterfARB");
    proc(pname, param);
}

GLAPI void APIENTRY glPointParameterfvARB(GLenum pname, const GLfloat *params)
{
    RESOLVE(PFNGLPOINTPARAMETERFVARBPROC, "glPointParameterfvARB");
    proc(pname, params);
}


GLAPI void APIENTRY glWindowPos3fARB(GLfloat x, GLfloat y, GLfloat z)
{
    RESOLVE(PFNGLWINDOWPOS3FARBPROC, "glWindowPos3fARB");
    proc(x, y, z);
}

GLAPI void APIENTRY glPointParameteri(GLenum pname, GLint param)
{
    RESOLVE(PFNGLPOINTPARAMETERIPROC, "glPointParameteri");
    proc(pname, param);
}

GLAPI void APIENTRY glPointParameteriv(GLenum pname, const GLint *params)
{
    RESOLVE(PFNGLPOINTPARAMETERIVPROC, "glPointParameteriv");
    proc(pname, params);
}

GLAPI void APIENTRY glPointParameteriNV(GLenum pname, GLint param)
{
    RESOLVE(PFNGLPOINTPARAMETERINVPROC, "glPointParameteriNV");
    proc(pname, param);
}

GLAPI void APIENTRY glPointParameterivNV(GLenum pname, const GLint *params)
{
    RESOLVE(PFNGLPOINTPARAMETERIVNVPROC, "glPointParameterivNV");
    proc(pname, params);
}

GLAPI void APIENTRY glSecondaryColor3bv(const GLbyte *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3BVPROC, "glSecondaryColor3bv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3dv(const GLdouble *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3DVPROC, "glSecondaryColor3dv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3fv(const GLfloat *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3FVPROC, "glSecondaryColor3fv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3iv(const GLint *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3IVPROC, "glSecondaryColor3iv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3sv(const GLshort *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3SVPROC, "glSecondaryColor3sv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3ubv(const GLubyte *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3UBVPROC, "glSecondaryColor3ubv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3uiv(const GLuint *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3UIVPROC, "glSecondaryColor3uiv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColor3usv(const GLushort *v)
{
    RESOLVE(PFNGLSECONDARYCOLOR3USVPROC, "glSecondaryColor3usv");
    proc(v);
}
GLAPI void APIENTRY glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    RESOLVE(PFNGLSECONDARYCOLORPOINTERPROC, "glSecondaryColorPointer");
    proc(size, type, stride, pointer);
}


GLAPI void APIENTRY glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    RESOLVE(PFNGLBLENDFUNCSEPARATEPROC, "glBlendFuncSeparate");
    proc(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
GLAPI void APIENTRY glFogCoordfv(const GLfloat *coord)
{
    RESOLVE(PFNGLFOGCOORDFVPROC, "glFogCoordfv");
    proc(coord);
}
GLAPI void APIENTRY glFogCoorddv(const GLdouble *coord)
{
    RESOLVE(PFNGLFOGCOORDDVPROC, "glFogCoorddv");
    proc(coord);
}
GLAPI void APIENTRY glFogCoordPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    RESOLVE(PFNGLFOGCOORDPOINTERPROC, "glFogCoordPointer");
    proc(type, stride, pointer);
}


GLAPI void APIENTRY glSampleCoverageARB(GLclampf value, GLboolean invert)
{
    RESOLVE(PFNGLSAMPLECOVERAGEARBPROC, "glSampleCoverageARB");
    proc(value, invert);
}
GLAPI void APIENTRY glSampleMaskSGIS(GLclampf value, GLboolean invert)
{
    RESOLVE(PFNGLSAMPLEMASKSGISPROC, "glSampleMaskSGIS");
    proc(value, invert);
}
GLAPI void APIENTRY glSamplePatternSGIS(GLenum pattern)
{
    RESOLVE(PFNGLSAMPLEPATTERNSGISPROC, "glSamplePatternSGIS");
    proc(pattern);
}

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

  // SET_Accum(disp, glAccum);
  // SET_AlphaFunc(disp, glAlphaFunc);
  // SET_AreTexturesResident(disp, glAreTexturesResident);
  // SET_ArrayElement(disp, glArrayElement);
  // SET_Begin(disp, glBegin);
  // SET_BindTexture(disp, glBindTexture);
  // SET_Bitmap(disp, glBitmap);
  // SET_BlendColor(disp, glBlendColor);
  // SET_BlendEquation(disp, glBlendEquation);
  // SET_BlendFunc(disp, glBlendFunc);
  // SET_CallList(disp, glCallList);
  //  SET_CallLists(disp, glCallLists);
  //  SET_Clear(disp, glClear);
  //  SET_ClearAccum(disp, glClearAccum);
  //  SET_ClearColor(disp, glClearColor);
  //  SET_ClearDepth(disp, glClearDepth);
  //  SET_ClearIndex(disp, glClearIndex);
  //  SET_ClearStencil(disp, glClearStencil);
  //  SET_ClipPlane(disp, glClipPlane);
  //  SET_Color3b(disp, glColor3b);
  //  SET_Color3bv(disp, glColor3bv);
/*   SET_Color3d(disp, glColor3d); */
/*   SET_Color3dv(disp, glColor3dv); */
/*   SET_Color3f(disp, glColor3f); */
/*   SET_Color3fv(disp, glColor3fv); */
/*   SET_Color3i(disp, glColor3i); */
/*   SET_Color3iv(disp, glColor3iv); */
/*   SET_Color3s(disp, glColor3s); */
/*   SET_Color3sv(disp, glColor3sv); */
/*   SET_Color3ub(disp, glColor3ub); */
/*   SET_Color3ubv(disp, glColor3ubv); */
/*   SET_Color3ui(disp, glColor3ui); */
/*   SET_Color3uiv(disp, glColor3uiv); */
/*   SET_Color3us(disp, glColor3us); */
/*   SET_Color3usv(disp, glColor3usv); */
/*   SET_Color4b(disp, glColor4b); */
/*   SET_Color4bv(disp, glColor4bv); */
/*   SET_Color4d(disp, glColor4d); */
/*   SET_Color4dv(disp, glColor4dv); */
/*   SET_Color4f(disp, glColor4f); */
/*   SET_Color4fv(disp, glColor4fv); */
/*   SET_Color4i(disp, glColor4i); */
/*   SET_Color4iv(disp, glColor4iv); */
/*   SET_Color4s(disp, glColor4s); */
/*   SET_Color4sv(disp, glColor4sv); */
/*   SET_Color4ub(disp, glColor4ub); */
/*   SET_Color4ubv(disp, glColor4ubv); */
/*   SET_Color4ui(disp, glColor4ui); */
/*   SET_Color4uiv(disp, glColor4uiv); */
/*   SET_Color4us(disp, glColor4us); */
/*   SET_Color4usv(disp, glColor4usv); */
//  SET_ColorMask(disp, glColorMask);
//  SET_ColorMaterial(disp, glColorMaterial);
//  SET_ColorPointer(disp, glColorPointer);
  SET_ColorSubTable(disp, glColorSubTable);
  SET_ColorTable(disp, glColorTable);
  SET_ColorTableParameterfv(disp, glColorTableParameterfv);
  SET_ColorTableParameteriv(disp, glColorTableParameteriv);
  SET_ConvolutionFilter1D(disp, glConvolutionFilter1D);
  SET_ConvolutionFilter2D(disp, glConvolutionFilter2D);
  SET_ConvolutionParameterf(disp, glConvolutionParameterf);
  SET_ConvolutionParameterfv(disp, glConvolutionParameterfv);
  SET_ConvolutionParameteri(disp, glConvolutionParameteri);
  SET_ConvolutionParameteriv(disp, glConvolutionParameteriv);
  SET_CopyColorSubTable(disp, glCopyColorSubTable);
  SET_CopyColorTable(disp, glCopyColorTable);
  SET_CopyConvolutionFilter1D(disp, glCopyConvolutionFilter1D);
  SET_CopyConvolutionFilter2D(disp, glCopyConvolutionFilter2D);
  //  SET_CopyPixels(disp, glCopyPixels);
  //  SET_CopyTexImage1D(disp, glCopyTexImage1D);
  //  SET_CopyTexImage2D(disp, glCopyTexImage2D);
  //  SET_CopyTexSubImage1D(disp, glCopyTexSubImage1D);
  //  SET_CopyTexSubImage2D(disp, glCopyTexSubImage2D);
  SET_CopyTexSubImage3D(disp, glCopyTexSubImage3D);
  //  SET_CullFace(disp, glCullFace);
  //  SET_DeleteLists(disp, glDeleteLists);
  //  SET_DeleteTextures(disp, glDeleteTextures);
  //  SET_DepthFunc(disp, glDepthFunc);
  //  SET_DepthMask(disp, glDepthMask);
  //  SET_DepthRange(disp, glDepthRange);
  //  SET_Disable(disp, glDisable);
  //  SET_DisableClientState(disp, glDisableClientState);
  //  SET_DrawArrays(disp, glDrawArrays);
  //  SET_DrawBuffer(disp, glDrawBuffer);
  //  SET_DrawElements(disp, glDrawElements);
  //  SET_DrawPixels(disp, glDrawPixels);
  //  SET_DrawRangeElements(disp, glDrawRangeElements);
  //  SET_EdgeFlag(disp, glEdgeFlag);
  //  SET_EdgeFlagPointer(disp, glEdgeFlagPointer);
  //  SET_EdgeFlagv(disp, glEdgeFlagv);
  //  SET_Enable(disp, glEnable);
  //  SET_EnableClientState(disp, glEnableClientState);
  //  SET_End(disp, glEnd);
  //  SET_EndList(disp, glEndList);
/*   SET_EvalCoord1d(disp, glEvalCoord1d); */
/*   SET_EvalCoord1dv(disp, glEvalCoord1dv); */
/*   SET_EvalCoord1f(disp, glEvalCoord1f); */
/*   SET_EvalCoord1fv(disp, glEvalCoord1fv); */
/*   SET_EvalCoord2d(disp, glEvalCoord2d); */
/*   SET_EvalCoord2dv(disp, glEvalCoord2dv); */
/*   SET_EvalCoord2f(disp, glEvalCoord2f); */
/*   SET_EvalCoord2fv(disp, glEvalCoord2fv); */
/*   SET_EvalMesh1(disp, glEvalMesh1); */
/*   SET_EvalMesh2(disp, glEvalMesh2); */
/*   SET_EvalPoint1(disp, glEvalPoint1); */
/*   SET_EvalPoint2(disp, glEvalPoint2); */
//  SET_FeedbackBuffer(disp, glFeedbackBuffer);
//  SET_Finish(disp, glFinish);
//  SET_Flush(disp, glFlush);
//  SET_Fogf(disp, glFogf);
//  SET_Fogfv(disp, glFogfv);
//  SET_Fogi(disp, glFogi);
///  SET_Fogiv(disp, glFogiv);
//  SET_FrontFace(disp, glFrontFace);
//  SET_Frustum(disp, glFrustum);
//  SET_GenLists(disp, glGenLists);
//  SET_GenTextures(disp, glGenTextures);
//  SET_GetBooleanv(disp, glGetBooleanv);
//  SET_GetClipPlane(disp, glGetClipPlane);
  SET_GetColorTable(disp, glGetColorTable);
  SET_GetColorTableParameterfv(disp, glGetColorTableParameterfv);
  SET_GetColorTableParameteriv(disp, glGetColorTableParameteriv);
  SET_GetConvolutionFilter(disp, glGetConvolutionFilter);
  SET_GetConvolutionParameterfv(disp, glGetConvolutionParameterfv);
  SET_GetConvolutionParameteriv(disp, glGetConvolutionParameteriv);
  //  SET_GetDoublev(disp, glGetDoublev);
  //  SET_GetError(disp, glGetError);
  //  SET_GetFloatv(disp, glGetFloatv);
  SET_GetHistogram(disp, glGetHistogram);
  SET_GetHistogramParameterfv(disp, glGetHistogramParameterfv);
  SET_GetHistogramParameteriv(disp, glGetHistogramParameteriv);
  //  SET_GetIntegerv(disp, glGetIntegerv);
  //  SET_GetLightfv(disp, glGetLightfv);
  //  SET_GetLightiv(disp, glGetLightiv);
  //  SET_GetMapdv(disp, glGetMapdv);
  //  SET_GetMapfv(disp, glGetMapfv);
  //  SET_GetMapiv(disp, glGetMapiv);
  //  SET_GetMaterialfv(disp, glGetMaterialfv);
  //  SET_GetMaterialiv(disp, glGetMaterialiv);
  SET_GetMinmax(disp, glGetMinmax);
  SET_GetMinmaxParameterfv(disp, glGetMinmaxParameterfv);
  SET_GetMinmaxParameteriv(disp, glGetMinmaxParameteriv);
  //  SET_GetPixelMapfv(disp, glGetPixelMapfv);
  //  SET_GetPixelMapuiv(disp, glGetPixelMapuiv);
  //  SET_GetPixelMapusv(disp, glGetPixelMapusv);
  //  SET_GetPointerv(disp, glGetPointerv);
  //  SET_GetPolygonStipple(disp, glGetPolygonStipple);
  SET_GetSeparableFilter(disp, glGetSeparableFilter);
  //  SET_GetString(disp, glGetString);
/*   SET_GetTexEnvfv(disp, glGetTexEnvfv); */
/*   SET_GetTexEnviv(disp, glGetTexEnviv); */
/*   SET_GetTexGendv(disp, glGetTexGendv); */
/*   SET_GetTexGenfv(disp, glGetTexGenfv); */
/*   SET_GetTexGeniv(disp, glGetTexGeniv); */
/*   SET_GetTexImage(disp, glGetTexImage); */
/*   SET_GetTexLevelParameterfv(disp, glGetTexLevelParameterfv); */
/*   SET_GetTexLevelParameteriv(disp, glGetTexLevelParameteriv); */
/*   SET_GetTexParameterfv(disp, glGetTexParameterfv); */
/*   SET_GetTexParameteriv(disp, glGetTexParameteriv); */
/*   SET_Hint(disp, glHint); */
  SET_Histogram(disp, glHistogram);
/*   SET_IndexMask(disp, glIndexMask); */
/*   SET_IndexPointer(disp, glIndexPointer); */
/*   SET_Indexd(disp, glIndexd); */
/*   SET_Indexdv(disp, glIndexdv); */
/*   SET_Indexf(disp, glIndexf); */
/*   SET_Indexfv(disp, glIndexfv); */
/*   SET_Indexi(disp, glIndexi); */
/*   SET_Indexiv(disp, glIndexiv); */
/*   SET_Indexs(disp, glIndexs); */
/*   SET_Indexsv(disp, glIndexsv); */
/*   SET_Indexub(disp, glIndexub); */
/*   SET_Indexubv(disp, glIndexubv); */
/*   SET_InitNames(disp, glInitNames); */
/*   SET_InterleavedArrays(disp, glInterleavedArrays); */
/*   SET_IsEnabled(disp, glIsEnabled); */
/*   SET_IsList(disp, glIsList); */
/*   SET_IsTexture(disp, glIsTexture); */
/*   SET_LightModelf(disp, glLightModelf); */
/*   SET_LightModelfv(disp, glLightModelfv); */
/*   SET_LightModeli(disp, glLightModeli); */
/*   SET_LightModeliv(disp, glLightModeliv); */
/*   SET_Lightf(disp, glLightf); */
/*   SET_Lightfv(disp, glLightfv); */
/*   SET_Lighti(disp, glLighti); */
/*   SET_Lightiv(disp, glLightiv); */
/*   SET_LineStipple(disp, glLineStipple); */
/*   SET_LineWidth(disp, glLineWidth); */
/*   SET_ListBase(disp, glListBase); */
/*   SET_LoadIdentity(disp, glLoadIdentity); */
/*   SET_LoadMatrixd(disp, glLoadMatrixd); */
/*   SET_LoadMatrixf(disp, glLoadMatrixf); */
/*   SET_LoadName(disp, glLoadName); */
/*   SET_LogicOp(disp, glLogicOp); */
/*   SET_Map1d(disp, glMap1d); */
/*   SET_Map1f(disp, glMap1f); */
/*   SET_Map2d(disp, glMap2d); */
/*   SET_Map2f(disp, glMap2f); */
/*   SET_MapGrid1d(disp, glMapGrid1d); */
/*   SET_MapGrid1f(disp, glMapGrid1f); */
/*   SET_MapGrid2d(disp, glMapGrid2d); */
/*   SET_MapGrid2f(disp, glMapGrid2f); */
/*   SET_Materialf(disp, glMaterialf); */
/*   SET_Materialfv(disp, glMaterialfv); */
/*   SET_Materiali(disp, glMateriali); */
/*   SET_Materialiv(disp, glMaterialiv); */
/*   SET_MatrixMode(disp, glMatrixMode); */
  SET_Minmax(disp, glMinmax);
/*   SET_MultMatrixd(disp, glMultMatrixd); */
/*   SET_MultMatrixf(disp, glMultMatrixf); */
/*   SET_NewList(disp, glNewList); */
/*   SET_Normal3b(disp, glNormal3b); */
/*   SET_Normal3bv(disp, glNormal3bv); */
/*   SET_Normal3d(disp, glNormal3d); */
/*   SET_Normal3dv(disp, glNormal3dv); */
/*   SET_Normal3f(disp, glNormal3f); */
/*   SET_Normal3fv(disp, glNormal3fv); */
/*   SET_Normal3i(disp, glNormal3i); */
/*   SET_Normal3iv(disp, glNormal3iv); */
/*   SET_Normal3s(disp, glNormal3s); */
/*   SET_Normal3sv(disp, glNormal3sv); */
/*   SET_NormalPointer(disp, glNormalPointer); */
/*   SET_Ortho(disp, glOrtho); */
/*   SET_PassThrough(disp, glPassThrough); */
/*   SET_PixelMapfv(disp, glPixelMapfv); */
/*   SET_PixelMapuiv(disp, glPixelMapuiv); */
/*   SET_PixelMapusv(disp, glPixelMapusv); */
/*   SET_PixelStoref(disp, glPixelStoref); */
/*   SET_PixelStorei(disp, glPixelStorei); */
/*   SET_PixelTransferf(disp, glPixelTransferf); */
/*   SET_PixelTransferi(disp, glPixelTransferi); */
/*   SET_PixelZoom(disp, glPixelZoom); */
/*   SET_PointSize(disp, glPointSize); */
/*   SET_PolygonMode(disp, glPolygonMode); */
/*   SET_PolygonOffset(disp, glPolygonOffset); */
/*   SET_PolygonStipple(disp, glPolygonStipple); */
/*   SET_PopAttrib(disp, glPopAttrib); */
/*   SET_PopClientAttrib(disp, glPopClientAttrib); */
/*   SET_PopMatrix(disp, glPopMatrix); */
/*   SET_PopName(disp, glPopName); */
/*   SET_PrioritizeTextures(disp, glPrioritizeTextures); */
/*   SET_PushAttrib(disp, glPushAttrib); */
/*   SET_PushClientAttrib(disp, glPushClientAttrib); */
/*   SET_PushMatrix(disp, glPushMatrix); */
/*   SET_PushName(disp, glPushName); */
/*   SET_RasterPos2d(disp, glRasterPos2d); */
/*   SET_RasterPos2dv(disp, glRasterPos2dv); */
/*   SET_RasterPos2f(disp, glRasterPos2f); */
/*   SET_RasterPos2fv(disp, glRasterPos2fv); */
/*   SET_RasterPos2i(disp, glRasterPos2i); */
/*   SET_RasterPos2iv(disp, glRasterPos2iv); */
/*   SET_RasterPos2s(disp, glRasterPos2s); */
/*   SET_RasterPos2sv(disp, glRasterPos2sv); */
/*   SET_RasterPos3d(disp, glRasterPos3d); */
/*   SET_RasterPos3dv(disp, glRasterPos3dv); */
/*   SET_RasterPos3f(disp, glRasterPos3f); */
/*   SET_RasterPos3fv(disp, glRasterPos3fv); */
/*   SET_RasterPos3i(disp, glRasterPos3i); */
/*   SET_RasterPos3iv(disp, glRasterPos3iv); */
/*   SET_RasterPos3s(disp, glRasterPos3s); */
/*   SET_RasterPos3sv(disp, glRasterPos3sv); */
/*   SET_RasterPos4d(disp, glRasterPos4d); */
/*   SET_RasterPos4dv(disp, glRasterPos4dv); */
/*   SET_RasterPos4f(disp, glRasterPos4f); */
/*   SET_RasterPos4fv(disp, glRasterPos4fv); */
/*   SET_RasterPos4i(disp, glRasterPos4i); */
/*   SET_RasterPos4iv(disp, glRasterPos4iv); */
/*   SET_RasterPos4s(disp, glRasterPos4s); */
/*   SET_RasterPos4sv(disp, glRasterPos4sv); */
/*   SET_ReadBuffer(disp, glReadBuffer); */
/*   SET_ReadPixels(disp, glReadPixels); */
/*   SET_Rectd(disp, glRectd); */
/*   SET_Rectdv(disp, glRectdv); */
/*   SET_Rectf(disp, glRectf); */
/*   SET_Rectfv(disp, glRectfv); */
/*   SET_Recti(disp, glRecti); */
/*   SET_Rectiv(disp, glRectiv); */
/*   SET_Rects(disp, glRects); */
/*   SET_Rectsv(disp, glRectsv); */
/*   SET_RenderMode(disp, glRenderMode); */
  SET_ResetHistogram(disp, glResetHistogram);
  SET_ResetMinmax(disp, glResetMinmax);
/*   SET_Rotated(disp, glRotated); */
/*   SET_Rotatef(disp, glRotatef); */
/*   SET_Scaled(disp, glScaled); */
/*   SET_Scalef(disp, glScalef); */
/*   SET_Scissor(disp, glScissor); */
/*   SET_SelectBuffer(disp, glSelectBuffer); */
  SET_SeparableFilter2D(disp, glSeparableFilter2D);
/*   SET_ShadeModel(disp, glShadeModel); */
/*   SET_StencilFunc(disp, glStencilFunc); */
/*   SET_StencilMask(disp, glStencilMask); */
/*   SET_StencilOp(disp, glStencilOp); */
/*   SET_TexCoord1d(disp, glTexCoord1d); */
/*   SET_TexCoord1dv(disp, glTexCoord1dv); */
/*   SET_TexCoord1f(disp, glTexCoord1f); */
/*   SET_TexCoord1fv(disp, glTexCoord1fv); */
/*   SET_TexCoord1i(disp, glTexCoord1i); */
/*   SET_TexCoord1iv(disp, glTexCoord1iv); */
/*   SET_TexCoord1s(disp, glTexCoord1s); */
/*   SET_TexCoord1sv(disp, glTexCoord1sv); */
/*   SET_TexCoord2d(disp, glTexCoord2d); */
/*   SET_TexCoord2dv(disp, glTexCoord2dv); */
/*   SET_TexCoord2f(disp, glTexCoord2f); */
/*   SET_TexCoord2fv(disp, glTexCoord2fv); */
/*   SET_TexCoord2i(disp, glTexCoord2i); */
/*   SET_TexCoord2iv(disp, glTexCoord2iv); */
/*   SET_TexCoord2s(disp, glTexCoord2s); */
/*   SET_TexCoord2sv(disp, glTexCoord2sv); */
/*   SET_TexCoord3d(disp, glTexCoord3d); */
/*   SET_TexCoord3dv(disp, glTexCoord3dv); */
/*   SET_TexCoord3f(disp, glTexCoord3f); */
/*   SET_TexCoord3fv(disp, glTexCoord3fv); */
/*   SET_TexCoord3i(disp, glTexCoord3i); */
/*   SET_TexCoord3iv(disp, glTexCoord3iv); */
/*   SET_TexCoord3s(disp, glTexCoord3s); */
/*   SET_TexCoord3sv(disp, glTexCoord3sv); */
/*   SET_TexCoord4d(disp, glTexCoord4d); */
/*   SET_TexCoord4dv(disp, glTexCoord4dv); */
/*   SET_TexCoord4f(disp, glTexCoord4f); */
/*   SET_TexCoord4fv(disp, glTexCoord4fv); */
/*   SET_TexCoord4i(disp, glTexCoord4i); */
/*   SET_TexCoord4iv(disp, glTexCoord4iv); */
/*   SET_TexCoord4s(disp, glTexCoord4s); */
/*   SET_TexCoord4sv(disp, glTexCoord4sv); */
/*   SET_TexCoordPointer(disp, glTexCoordPointer); */
/*   SET_TexEnvf(disp, glTexEnvf); */
/*   SET_TexEnvfv(disp, glTexEnvfv); */
/*   SET_TexEnvi(disp, glTexEnvi); */
/*   SET_TexEnviv(disp, glTexEnviv); */
/*   SET_TexGend(disp, glTexGend); */
/*   SET_TexGendv(disp, glTexGendv); */
/*   SET_TexGenf(disp, glTexGenf); */
/*   SET_TexGenfv(disp, glTexGenfv); */
/*   SET_TexGeni(disp, glTexGeni); */
/*   SET_TexGeniv(disp, glTexGeniv); */
  //  SET_TexImage1D(disp, glTexImage1D);
  //  SET_TexImage2D(disp, glTexImage2D);
  SET_TexImage3D(disp, glTexImage3D);
/*   SET_TexParameterf(disp, glTexParameterf); */
/*   SET_TexParameterfv(disp, glTexParameterfv); */
/*   SET_TexParameteri(disp, glTexParameteri); */
/*   SET_TexParameteriv(disp, glTexParameteriv); */
  //  SET_TexSubImage1D(disp, glTexSubImage1D);
  //  SET_TexSubImage2D(disp, glTexSubImage2D);
  SET_TexSubImage3D(disp, glTexSubImage3D);
/*   SET_Translated(disp, glTranslated); */
/*   SET_Translatef(disp, glTranslatef); */
/*   SET_Vertex2d(disp, glVertex2d); */
/*   SET_Vertex2dv(disp, glVertex2dv); */
/*   SET_Vertex2f(disp, glVertex2f); */
/*   SET_Vertex2fv(disp, glVertex2fv); */
/*   SET_Vertex2i(disp, glVertex2i); */
/*   SET_Vertex2iv(disp, glVertex2iv); */
/*   SET_Vertex2s(disp, glVertex2s); */
/*   SET_Vertex2sv(disp, glVertex2sv); */
/*   SET_Vertex3d(disp, glVertex3d); */
/*   SET_Vertex3dv(disp, glVertex3dv); */
/*   SET_Vertex3f(disp, glVertex3f); */
/*   SET_Vertex3fv(disp, glVertex3fv); */
/*   SET_Vertex3i(disp, glVertex3i); */
/*   SET_Vertex3iv(disp, glVertex3iv); */
/*   SET_Vertex3s(disp, glVertex3s); */
/*   SET_Vertex3sv(disp, glVertex3sv); */
/*   SET_Vertex4d(disp, glVertex4d); */
/*   SET_Vertex4dv(disp, glVertex4dv); */
/*   SET_Vertex4f(disp, glVertex4f); */
/*   SET_Vertex4fv(disp, glVertex4fv); */
/*   SET_Vertex4i(disp, glVertex4i); */
/*   SET_Vertex4iv(disp, glVertex4iv); */
/*   SET_Vertex4s(disp, glVertex4s); */
/*   SET_Vertex4sv(disp, glVertex4sv); */
/*   SET_VertexPointer(disp, glVertexPointer); */
/*   SET_Viewport(disp, glViewport); */

#if GL_VERSION_2_0
/*   SET_AttachShader(disp, glAttachShader); */
/*   SET_DeleteShader(disp, glDeleteShader); */
/*   SET_DetachShader(disp, glDetachShader); */
/*   SET_GetAttachedShaders(disp, glGetAttachedShaders); */
/*   SET_GetProgramInfoLog(disp, glGetProgramInfoLog); */
/*   SET_GetShaderInfoLog(disp, glGetShaderInfoLog); */
/*   SET_GetShaderiv(disp, glGetShaderiv); */
/*   SET_IsShader(disp, glIsShader); */
/*   SET_StencilFuncSeparate(disp, glStencilFuncSeparate); */
/*   SET_StencilMaskSeparate(disp, glStencilMaskSeparate); */
/*   SET_StencilOpSeparate(disp, glStencilOpSeparate); */
#endif

#if GL_VERSION_2_1
/*   SET_UniformMatrix2x3fv(disp, glUniformMatrix2x3fv); */
/*   SET_UniformMatrix2x4fv(disp, glUniformMatrix2x4fv); */
/*   SET_UniformMatrix3x2fv(disp, glUniformMatrix3x2fv); */
/*   SET_UniformMatrix3x4fv(disp, glUniformMatrix3x4fv); */
/*   SET_UniformMatrix4x2fv(disp, glUniformMatrix4x2fv); */
/*   SET_UniformMatrix4x3fv(disp, glUniformMatrix4x3fv); */
#endif

#if GL_APPLE_vertex_array_object
/*   SET_BindVertexArrayAPPLE(disp, glBindVertexArrayAPPLE); */
/*   SET_DeleteVertexArraysAPPLE(disp, glDeleteVertexArraysAPPLE); */
/*   SET_GenVertexArraysAPPLE(disp, glGenVertexArraysAPPLE); */
/*   SET_IsVertexArrayAPPLE(disp, glIsVertexArrayAPPLE); */
#endif

#if GL_ARB_draw_buffers
/*   SET_DrawBuffersARB(disp, glDrawBuffersARB); */
#endif

#if GL_ARB_multisample
/*   SET_SampleCoverageARB(disp, glSampleCoverageARB); */
#endif

#if GL_ARB_multitexture
  SET_ActiveTextureARB(disp, glActiveTextureARB);
/*   SET_ClientActiveTextureARB(disp, glClientActiveTextureARB); */
/*    SET_MultiTexCoord1dARB(disp, glMultiTexCoord1dARB); */
  SET_MultiTexCoord1dvARB(disp, glMultiTexCoord1dvARB);
/*   SET_MultiTexCoord1fARB(disp, glMultiTexCoord1fARB); */
  SET_MultiTexCoord1fvARB(disp, glMultiTexCoord1fvARB);
/*   SET_MultiTexCoord1iARB(disp, glMultiTexCoord1iARB); */
  SET_MultiTexCoord1ivARB(disp, glMultiTexCoord1ivARB);
/*   SET_MultiTexCoord1sARB(disp, glMultiTexCoord1sARB); */
  SET_MultiTexCoord1svARB(disp, glMultiTexCoord1svARB);
/*   SET_MultiTexCoord2dARB(disp, glMultiTexCoord2dARB); */
  SET_MultiTexCoord2dvARB(disp, glMultiTexCoord2dvARB);
/*   SET_MultiTexCoord2fARB(disp, glMultiTexCoord2fARB); */
  SET_MultiTexCoord2fvARB(disp, glMultiTexCoord2fvARB);
/*   SET_MultiTexCoord2iARB(disp, glMultiTexCoord2iARB); */
  SET_MultiTexCoord2ivARB(disp, glMultiTexCoord2ivARB);
/*   SET_MultiTexCoord2sARB(disp, glMultiTexCoord2sARB); */
  SET_MultiTexCoord2svARB(disp, glMultiTexCoord2svARB);
/*   SET_MultiTexCoord3dARB(disp, glMultiTexCoord3dARB); */
  SET_MultiTexCoord3dvARB(disp, glMultiTexCoord3dvARB);
/*   SET_MultiTexCoord3fARB(disp, glMultiTexCoord3fARB); */
  SET_MultiTexCoord3fvARB(disp, glMultiTexCoord3fvARB);
/*   SET_MultiTexCoord3iARB(disp, glMultiTexCoord3iARB); */
  SET_MultiTexCoord3ivARB(disp, glMultiTexCoord3ivARB);
/*   SET_MultiTexCoord3sARB(disp, glMultiTexCoord3sARB); */
  SET_MultiTexCoord3svARB(disp, glMultiTexCoord3svARB);
/*   SET_MultiTexCoord4dARB(disp, glMultiTexCoord4dARB); */
  SET_MultiTexCoord4dvARB(disp, glMultiTexCoord4dvARB);
/*   SET_MultiTexCoord4fARB(disp, glMultiTexCoord4fARB); */
  SET_MultiTexCoord4fvARB(disp, glMultiTexCoord4fvARB);
/*   SET_MultiTexCoord4iARB(disp, glMultiTexCoord4iARB); */
  SET_MultiTexCoord4ivARB(disp, glMultiTexCoord4ivARB);
/*   SET_MultiTexCoord4sARB(disp, glMultiTexCoord4sARB); */
  SET_MultiTexCoord4svARB(disp, glMultiTexCoord4svARB);
#endif

#if GL_ARB_occlusion_query
/*   SET_BeginQueryARB(disp, glBeginQueryARB); */
/*   SET_DeleteQueriesARB(disp, glDeleteQueriesARB); */
/*   SET_EndQueryARB(disp, glEndQueryARB); */
/*   SET_GenQueriesARB(disp, glGenQueriesARB); */
/*   SET_GetQueryObjectivARB(disp, glGetQueryObjectivARB); */
/*   SET_GetQueryObjectuivARB(disp, glGetQueryObjectuivARB); */
/*   SET_GetQueryivARB(disp, glGetQueryivARB); */
/*   SET_IsQueryARB(disp, glIsQueryARB); */
#endif

#if GL_ARB_shader_objects
/*   SET_AttachObjectARB(disp, glAttachObjectARB); */
/*   SET_CompileShaderARB(disp, glCompileShaderARB); */
/*   SET_DeleteObjectARB(disp, glDeleteObjectARB); */
/*   SET_GetHandleARB(disp, glGetHandleARB); */
/*   SET_DetachObjectARB(disp, glDetachObjectARB); */
/*   SET_CreateProgramObjectARB(disp, glCreateProgramObjectARB); */
/*   SET_CreateShaderObjectARB(disp, glCreateShaderObjectARB); */
/*   SET_GetInfoLogARB(disp, glGetInfoLogARB); */
/*   SET_GetActiveUniformARB(disp, glGetActiveUniformARB); */
/*   SET_GetAttachedObjectsARB(disp, glGetAttachedObjectsARB); */
/*   SET_GetObjectParameterfvARB(disp, glGetObjectParameterfvARB); */
/*   SET_GetObjectParameterivARB(disp, glGetObjectParameterivARB); */
/*   SET_GetShaderSourceARB(disp, glGetShaderSourceARB); */
/*   SET_GetUniformLocationARB(disp, glGetUniformLocationARB); */
/*   SET_GetUniformfvARB(disp, glGetUniformfvARB); */
/*   SET_GetUniformivARB(disp, glGetUniformivARB); */
/*   SET_LinkProgramARB(disp, glLinkProgramARB); */
/*   SET_ShaderSourceARB(disp, glShaderSourceARB); */
/*   SET_Uniform1fARB(disp, glUniform1fARB); */
/*   SET_Uniform1fvARB(disp, glUniform1fvARB); */
/*   SET_Uniform1iARB(disp, glUniform1iARB); */
/*   SET_Uniform1ivARB(disp, glUniform1ivARB); */
/*   SET_Uniform2fARB(disp, glUniform2fARB); */
/*   SET_Uniform2fvARB(disp, glUniform2fvARB); */
/*   SET_Uniform2iARB(disp, glUniform2iARB); */
/*   SET_Uniform2ivARB(disp, glUniform2ivARB); */
/*   SET_Uniform3fARB(disp, glUniform3fARB); */
/*   SET_Uniform3fvARB(disp, glUniform3fvARB); */
/*   SET_Uniform3iARB(disp, glUniform3iARB); */
/*   SET_Uniform3ivARB(disp, glUniform3ivARB); */
/*   SET_Uniform4fARB(disp, glUniform4fARB); */
/*   SET_Uniform4fvARB(disp, glUniform4fvARB); */
/*   SET_Uniform4iARB(disp, glUniform4iARB); */
/*   SET_Uniform4ivARB(disp, glUniform4ivARB); */
/*   SET_UniformMatrix2fvARB(disp, glUniformMatrix2fvARB); */
/*   SET_UniformMatrix3fvARB(disp, glUniformMatrix3fvARB); */
/*   SET_UniformMatrix4fvARB(disp, glUniformMatrix4fvARB); */
/*   SET_UseProgramObjectARB(disp, glUseProgramObjectARB); */
/*   SET_ValidateProgramARB(disp, glValidateProgramARB); */
#endif

#if GL_ARB_texture_compression
/*   SET_CompressedTexImage1DARB(disp, glCompressedTexImage1DARB); */
/*   SET_CompressedTexImage2DARB(disp, glCompressedTexImage2DARB); */
/*   SET_CompressedTexImage3DARB(disp, glCompressedTexImage3DARB); */
/*   SET_CompressedTexSubImage1DARB(disp, glCompressedTexSubImage1DARB); */
/*   SET_CompressedTexSubImage2DARB(disp, glCompressedTexSubImage2DARB); */
/*   SET_CompressedTexSubImage3DARB(disp, glCompressedTexSubImage3DARB); */
/*   SET_GetCompressedTexImageARB(disp, glGetCompressedTexImageARB); */
#endif

#if GL_ARB_transpose_matrix
/*   SET_LoadTransposeMatrixdARB(disp, glLoadTransposeMatrixdARB); */
/*   SET_LoadTransposeMatrixfARB(disp, glLoadTransposeMatrixfARB); */
/*   SET_MultTransposeMatrixdARB(disp, glMultTransposeMatrixdARB); */
/*   SET_MultTransposeMatrixfARB(disp, glMultTransposeMatrixfARB); */
#endif

#if GL_ARB_vertex_buffer_object
/*   SET_BindBufferARB(disp, glBindBufferARB); */
/*   SET_BufferDataARB(disp, glBufferDataARB); */
/*   SET_BufferSubDataARB(disp, glBufferSubDataARB); */
/*   SET_DeleteBuffersARB(disp, glDeleteBuffersARB); */
/*   SET_GenBuffersARB(disp, glGenBuffersARB); */
/*   SET_GetBufferParameterivARB(disp, glGetBufferParameterivARB); */
/*   SET_GetBufferPointervARB(disp, glGetBufferPointervARB); */
/*   SET_GetBufferSubDataARB(disp, glGetBufferSubDataARB); */
/*   SET_IsBufferARB(disp, glIsBufferARB); */
/*   SET_MapBufferARB(disp, glMapBufferARB); */
/*   SET_UnmapBufferARB(disp, glUnmapBufferARB); */
#endif

#if GL_ARB_vertex_program
/*   SET_DisableVertexAttribArrayARB(disp, glDisableVertexAttribArrayARB); */
/*   SET_EnableVertexAttribArrayARB(disp, glEnableVertexAttribArrayARB); */
/*   SET_GetProgramEnvParameterdvARB(disp, glGetProgramEnvParameterdvARB); */
/*   SET_GetProgramEnvParameterfvARB(disp, glGetProgramEnvParameterfvARB); */
/*   SET_GetProgramLocalParameterdvARB(disp, glGetProgramLocalParameterdvARB); */
/*   SET_GetProgramLocalParameterfvARB(disp, glGetProgramLocalParameterfvARB); */
/*   SET_GetProgramStringARB(disp, glGetProgramStringARB); */
/*   SET_GetProgramivARB(disp, glGetProgramivARB); */
/*   SET_GetVertexAttribdvARB(disp, glGetVertexAttribdvARB); */
/*   SET_GetVertexAttribfvARB(disp, glGetVertexAttribfvARB); */
/*   SET_GetVertexAttribivARB(disp, glGetVertexAttribivARB); */
/*   SET_ProgramEnvParameter4dARB(disp, glProgramEnvParameter4dARB); */
/*   SET_ProgramEnvParameter4dvARB(disp, glProgramEnvParameter4dvARB); */
/*   SET_ProgramEnvParameter4fARB(disp, glProgramEnvParameter4fARB); */
/*   SET_ProgramEnvParameter4fvARB(disp, glProgramEnvParameter4fvARB); */
/*   SET_ProgramLocalParameter4dARB(disp, glProgramLocalParameter4dARB); */
/*   SET_ProgramLocalParameter4dvARB(disp, glProgramLocalParameter4dvARB); */
/*   SET_ProgramLocalParameter4fARB(disp, glProgramLocalParameter4fARB); */
/*   SET_ProgramLocalParameter4fvARB(disp, glProgramLocalParameter4fvARB); */
/*   SET_ProgramStringARB(disp, glProgramStringARB); */
/*   SET_VertexAttrib1dARB(disp, glVertexAttrib1dARB); */
/*   SET_VertexAttrib1dvARB(disp, glVertexAttrib1dvARB); */
/*   SET_VertexAttrib1fARB(disp, glVertexAttrib1fARB); */
/*   SET_VertexAttrib1fvARB(disp, glVertexAttrib1fvARB); */
/*   SET_VertexAttrib1sARB(disp, glVertexAttrib1sARB); */
/*   SET_VertexAttrib1svARB(disp, glVertexAttrib1svARB); */
/*   SET_VertexAttrib2dARB(disp, glVertexAttrib2dARB); */
/*   SET_VertexAttrib2dvARB(disp, glVertexAttrib2dvARB); */
/*   SET_VertexAttrib2fARB(disp, glVertexAttrib2fARB); */
/*   SET_VertexAttrib2fvARB(disp, glVertexAttrib2fvARB); */
/*   SET_VertexAttrib2sARB(disp, glVertexAttrib2sARB); */
/*   SET_VertexAttrib2svARB(disp, glVertexAttrib2svARB); */
/*   SET_VertexAttrib3dARB(disp, glVertexAttrib3dARB); */
/*   SET_VertexAttrib3dvARB(disp, glVertexAttrib3dvARB); */
/*   SET_VertexAttrib3fARB(disp, glVertexAttrib3fARB); */
/*   SET_VertexAttrib3fvARB(disp, glVertexAttrib3fvARB); */
/*   SET_VertexAttrib3sARB(disp, glVertexAttrib3sARB); */
/*   SET_VertexAttrib3svARB(disp, glVertexAttrib3svARB); */
/*   SET_VertexAttrib4NbvARB(disp, glVertexAttrib4NbvARB); */
/*   SET_VertexAttrib4NivARB(disp, glVertexAttrib4NivARB); */
/*   SET_VertexAttrib4NsvARB(disp, glVertexAttrib4NsvARB); */
/*   SET_VertexAttrib4NubARB(disp, glVertexAttrib4NubARB); */
/*   SET_VertexAttrib4NubvARB(disp, glVertexAttrib4NubvARB); */
/*   SET_VertexAttrib4NuivARB(disp, glVertexAttrib4NuivARB); */
/*   SET_VertexAttrib4NusvARB(disp, glVertexAttrib4NusvARB); */
/*   SET_VertexAttrib4bvARB(disp, glVertexAttrib4bvARB); */
/*   SET_VertexAttrib4dARB(disp, glVertexAttrib4dARB); */
/*   SET_VertexAttrib4dvARB(disp, glVertexAttrib4dvARB); */
/*   SET_VertexAttrib4fARB(disp, glVertexAttrib4fARB); */
/*   SET_VertexAttrib4fvARB(disp, glVertexAttrib4fvARB); */
/*   SET_VertexAttrib4ivARB(disp, glVertexAttrib4ivARB); */
/*   SET_VertexAttrib4sARB(disp, glVertexAttrib4sARB); */
/*   SET_VertexAttrib4svARB(disp, glVertexAttrib4svARB); */
/*   SET_VertexAttrib4ubvARB(disp, glVertexAttrib4ubvARB); */
/*   SET_VertexAttrib4uivARB(disp, glVertexAttrib4uivARB); */
/*   SET_VertexAttrib4usvARB(disp, glVertexAttrib4usvARB); */
/*   SET_VertexAttribPointerARB(disp, glVertexAttribPointerARB); */
#endif

#if GL_ARB_vertex_shader
/*   SET_BindAttribLocationARB(disp, glBindAttribLocationARB); */
/*   SET_GetActiveAttribARB(disp, glGetActiveAttribARB); */
/*   SET_GetAttribLocationARB(disp, glGetAttribLocationARB); */
#endif

#if GL_ARB_window_pos
/*   SET_WindowPos2dMESA(disp, glWindowPos2dARB); */
/*   SET_WindowPos2dvMESA(disp, glWindowPos2dvARB); */
/*   SET_WindowPos2fMESA(disp, glWindowPos2fARB); */
/*   SET_WindowPos2fvMESA(disp, glWindowPos2fvARB); */
/*   SET_WindowPos2iMESA(disp, glWindowPos2iARB); */
/*   SET_WindowPos2ivMESA(disp, glWindowPos2ivARB); */
/*   SET_WindowPos2sMESA(disp, glWindowPos2sARB); */
/*   SET_WindowPos2svMESA(disp, glWindowPos2svARB); */
/*   SET_WindowPos3dMESA(disp, glWindowPos3dARB); */
/*   SET_WindowPos3dvMESA(disp, glWindowPos3dvARB); */
/*   SET_WindowPos3fMESA(disp, glWindowPos3fARB); */
/*   SET_WindowPos3fvMESA(disp, glWindowPos3fvARB); */
/*   SET_WindowPos3iMESA(disp, glWindowPos3iARB); */
/*   SET_WindowPos3ivMESA(disp, glWindowPos3ivARB); */
/*   SET_WindowPos3sMESA(disp, glWindowPos3sARB); */
/*   SET_WindowPos3svMESA(disp, glWindowPos3svARB); */
#endif

#if GL_ATI_fragment_shader
/*   SET_AlphaFragmentOp1ATI(disp, glAlphaFragmentOp1ATI); */
/*   SET_AlphaFragmentOp2ATI(disp, glAlphaFragmentOp2ATI); */
/*   SET_AlphaFragmentOp3ATI(disp, glAlphaFragmentOp3ATI); */
/*   SET_BeginFragmentShaderATI(disp, glBeginFragmentShaderATI); */
/*   SET_BindFragmentShaderATI(disp, glBindFragmentShaderATI); */
/*   SET_ColorFragmentOp1ATI(disp, glColorFragmentOp1ATI); */
/*   SET_ColorFragmentOp2ATI(disp, glColorFragmentOp2ATI); */
/*   SET_ColorFragmentOp3ATI(disp, glColorFragmentOp3ATI); */
/*   SET_DeleteFragmentShaderATI(disp, glDeleteFragmentShaderATI); */
/*   SET_EndFragmentShaderATI(disp, glEndFragmentShaderATI); */
/*   SET_GenFragmentShadersATI(disp, glGenFragmentShadersATI); */
/*   SET_PassTexCoordATI(disp, glPassTexCoordATI); */
/*   SET_SampleMapATI(disp, glSampleMapATI); */
/*   SET_SetFragmentShaderConstantATI(disp, glSetFragmentShaderConstantATI); */
#elif GL_EXT_fragment_shader
  SET_AlphaFragmentOp1ATI(disp, glAlphaFragmentOp1EXT);
  SET_AlphaFragmentOp2ATI(disp, glAlphaFragmentOp2EXT);
  SET_AlphaFragmentOp3ATI(disp, glAlphaFragmentOp3EXT);
  SET_BeginFragmentShaderATI(disp, glBeginFragmentShaderEXT);
  SET_BindFragmentShaderATI(disp, glBindFragmentShaderEXT);
  SET_ColorFragmentOp1ATI(disp, glColorFragmentOp1EXT);
  SET_ColorFragmentOp2ATI(disp, glColorFragmentOp2EXT);
  SET_ColorFragmentOp3ATI(disp, glColorFragmentOp3EXT);
  SET_DeleteFragmentShaderATI(disp, glDeleteFragmentShaderEXT);
  SET_EndFragmentShaderATI(disp, glEndFragmentShaderEXT);
  SET_GenFragmentShadersATI(disp, glGenFragmentShadersEXT);
  SET_PassTexCoordATI(disp, glPassTexCoordEXT);
  SET_SampleMapATI(disp, glSampleMapEXT);
  SET_SetFragmentShaderConstantATI(disp, glSetFragmentShaderConstantEXT);
#endif

#if GL_ATI_separate_stencil
/*   SET_StencilFuncSeparateATI(disp, glStencilFuncSeparateATI); */
#endif

#if GL_EXT_blend_equation_separate
/*   SET_BlendEquationSeparateEXT(disp, glBlendEquationSeparateEXT); */
#endif

#if GL_EXT_blend_func_separate
/*   SET_BlendFuncSeparateEXT(disp, glBlendFuncSeparateEXT); */
#endif

#if GL_EXT_depth_bounds_test
/*   SET_DepthBoundsEXT(disp, glDepthBoundsEXT); */
#endif

#if GL_EXT_compiled_vertex_array
/*   SET_LockArraysEXT(disp, glLockArraysEXT); */
/*   SET_UnlockArraysEXT(disp, glUnlockArraysEXT); */
#endif

#if GL_EXT_cull_vertex
/*   SET_CullParameterdvEXT(disp, glCullParameterdvEXT); */
/*   SET_CullParameterfvEXT(disp, glCullParameterfvEXT); */
#endif

#if GL_EXT_fog_coord
/*   SET_FogCoordPointerEXT(disp, glFogCoordPointerEXT); */
/*   SET_FogCoorddEXT(disp, glFogCoorddEXT); */
/*   SET_FogCoorddvEXT(disp, glFogCoorddvEXT); */
/*   SET_FogCoordfEXT(disp, glFogCoordfEXT); */
/*   SET_FogCoordfvEXT(disp, glFogCoordfvEXT); */
#endif

#if GL_EXT_framebuffer_blit
/*   SET_BlitFramebufferEXT(disp, glBlitFramebufferEXT); */
#endif

#if GL_EXT_framebuffer_object
/*   SET_BindFramebufferEXT(disp, glBindFramebufferEXT); */
/*   SET_BindRenderbufferEXT(disp, glBindRenderbufferEXT); */
/*   SET_CheckFramebufferStatusEXT(disp, glCheckFramebufferStatusEXT); */
/*   SET_DeleteFramebuffersEXT(disp, glDeleteFramebuffersEXT); */
/*   SET_DeleteRenderbuffersEXT(disp, glDeleteRenderbuffersEXT); */
/*   SET_FramebufferRenderbufferEXT(disp, glFramebufferRenderbufferEXT); */
/*   SET_FramebufferTexture1DEXT(disp, glFramebufferTexture1DEXT); */
/*   SET_FramebufferTexture2DEXT(disp, glFramebufferTexture2DEXT); */
/*   SET_FramebufferTexture3DEXT(disp, glFramebufferTexture3DEXT); */
/*   SET_GenerateMipmapEXT(disp, glGenerateMipmapEXT); */
/*   SET_GenFramebuffersEXT(disp, glGenFramebuffersEXT); */
/*   SET_GenRenderbuffersEXT(disp, glGenRenderbuffersEXT); */
/*   SET_GetFramebufferAttachmentParameterivEXT(disp, glGetFramebufferAttachmentParameterivEXT); */
/*   SET_GetRenderbufferParameterivEXT(disp, glGetRenderbufferParameterivEXT); */
/*   SET_IsFramebufferEXT(disp, glIsFramebufferEXT); */
/*   SET_IsRenderbufferEXT(disp, glIsRenderbufferEXT); */
/*   SET_RenderbufferStorageEXT(disp, glRenderbufferStorageEXT); */
#endif

#if GL_EXT_gpu_program_parameters
/*   SET_ProgramEnvParameters4fvEXT(disp, glProgramEnvParameters4fvEXT); */
/*   SET_ProgramLocalParameters4fvEXT(disp, glProgramLocalParameters4fvEXT); */
#endif

#if GL_EXT_multi_draw_arrays
/*   SET_MultiDrawArraysEXT(disp, glMultiDrawArraysEXT); */
/*   SET_MultiDrawElementsEXT(disp, glMultiDrawElementsEXT); */
#endif

#if GL_EXT_point_parameters
/*   SET_PointParameterfEXT(disp, glPointParameterfEXT); */
/*   SET_PointParameterfvEXT(disp, glPointParameterfvEXT); */
#elif GL_ARB_point_parameters
  SET_PointParameterfEXT(disp, glPointParameterfARB);
  SET_PointParameterfvEXT(disp, glPointParameterfvARB);
#endif

#if GL_EXT_polygon_offset
/*   SET_PolygonOffsetEXT(disp, glPolygonOffsetEXT); */
#endif

#if GL_EXT_secondary_color
/*   SET_SecondaryColor3bEXT(disp, glSecondaryColor3bEXT); */
/*   SET_SecondaryColor3bvEXT(disp, glSecondaryColor3bvEXT); */
/*   SET_SecondaryColor3dEXT(disp, glSecondaryColor3dEXT); */
/*   SET_SecondaryColor3dvEXT(disp, glSecondaryColor3dvEXT); */
/*   SET_SecondaryColor3fEXT(disp, glSecondaryColor3fEXT); */
/*   SET_SecondaryColor3fvEXT(disp, glSecondaryColor3fvEXT); */
/*   SET_SecondaryColor3iEXT(disp, glSecondaryColor3iEXT); */
/*   SET_SecondaryColor3ivEXT(disp, glSecondaryColor3ivEXT); */
/*   SET_SecondaryColor3sEXT(disp, glSecondaryColor3sEXT); */
/*   SET_SecondaryColor3svEXT(disp, glSecondaryColor3svEXT); */
/*   SET_SecondaryColor3ubEXT(disp, glSecondaryColor3ubEXT); */
/*   SET_SecondaryColor3ubvEXT(disp, glSecondaryColor3ubvEXT); */
/*   SET_SecondaryColor3uiEXT(disp, glSecondaryColor3uiEXT); */
/*   SET_SecondaryColor3uivEXT(disp, glSecondaryColor3uivEXT); */
/*   SET_SecondaryColor3usEXT(disp, glSecondaryColor3usEXT); */
/*   SET_SecondaryColor3usvEXT(disp, glSecondaryColor3usvEXT); */
/*   SET_SecondaryColorPointerEXT(disp, glSecondaryColorPointerEXT); */
#endif

#if GL_EXT_stencil_two_side
  SET_ActiveStencilFaceEXT(disp, glActiveStencilFaceEXT);
#endif

#if GL_EXT_timer_query
/*   SET_GetQueryObjecti64vEXT(disp, glGetQueryObjecti64vEXT); */
/*   SET_GetQueryObjectui64vEXT(disp, glGetQueryObjectui64vEXT); */
#endif

#if GL_EXT_vertex_array
/*   SET_ColorPointerEXT(disp, glColorPointerEXT); */
/*   SET_EdgeFlagPointerEXT(disp, glEdgeFlagPointerEXT); */
/*   SET_IndexPointerEXT(disp, glIndexPointerEXT); */
/*   SET_NormalPointerEXT(disp, glNormalPointerEXT); */
/*   SET_TexCoordPointerEXT(disp, glTexCoordPointerEXT); */
/*   SET_VertexPointerEXT(disp, glVertexPointerEXT); */
#endif

#if GL_IBM_multimode_draw_arrays
/*   SET_MultiModeDrawArraysIBM(disp, glMultiModeDrawArraysIBM); */
/*   SET_MultiModeDrawElementsIBM(disp, glMultiModeDrawElementsIBM); */
#endif

#if GL_MESA_resize_buffers
/*   SET_ResizeBuffersMESA(disp, glResizeBuffersMESA); */
#endif

#if GL_MESA_window_pos
/*   SET_WindowPos4dMESA(disp, glWindowPos4dMESA); */
/*   SET_WindowPos4dvMESA(disp, glWindowPos4dvMESA); */
/*   SET_WindowPos4fMESA(disp, glWindowPos4fMESA); */
/*   SET_WindowPos4fvMESA(disp, glWindowPos4fvMESA); */
/*   SET_WindowPos4iMESA(disp, glWindowPos4iMESA); */
/*   SET_WindowPos4ivMESA(disp, glWindowPos4ivMESA); */
/*   SET_WindowPos4sMESA(disp, glWindowPos4sMESA); */
/*   SET_WindowPos4svMESA(disp, glWindowPos4svMESA); */
#endif

#if GL_NV_fence
/*   SET_DeleteFencesNV(disp, glDeleteFencesNV); */
/*   SET_FinishFenceNV(disp, glFinishFenceNV); */
/*   SET_GenFencesNV(disp, glGenFencesNV); */
/*   SET_GetFenceivNV(disp, glGetFenceivNV); */
/*   SET_IsFenceNV(disp, glIsFenceNV); */
/*   SET_SetFenceNV(disp, glSetFenceNV); */
/*   SET_TestFenceNV(disp, glTestFenceNV); */
#endif

#if GL_NV_fragment_program
/*   SET_GetProgramNamedParameterdvNV(disp, glGetProgramNamedParameterdvNV); */
/*   SET_GetProgramNamedParameterfvNV(disp, glGetProgramNamedParameterfvNV); */
/*   SET_ProgramNamedParameter4dNV(disp, glProgramNamedParameter4dNV); */
/*   SET_ProgramNamedParameter4dvNV(disp, glProgramNamedParameter4dvNV); */
/*   SET_ProgramNamedParameter4fNV(disp, glProgramNamedParameter4fNV); */
/*   SET_ProgramNamedParameter4fvNV(disp, glProgramNamedParameter4fvNV); */
#endif

#if GL_NV_geometry_program4
/*   SET_FramebufferTextureLayerEXT(disp, glFramebufferTextureLayerEXT); */
#endif

#if GL_NV_point_sprite
  SET_PointParameteriNV(disp, glPointParameteriNV);
  SET_PointParameterivNV(disp, glPointParameterivNV);
#endif

#if GL_NV_register_combiners
/*   SET_CombinerInputNV(disp, glCombinerInputNV); */
/*   SET_CombinerOutputNV(disp, glCombinerOutputNV); */
/*   SET_CombinerParameterfNV(disp, glCombinerParameterfNV); */
/*   SET_CombinerParameterfvNV(disp, glCombinerParameterfvNV); */
/*   SET_CombinerParameteriNV(disp, glCombinerParameteriNV); */
/*   SET_CombinerParameterivNV(disp, glCombinerParameterivNV); */
/*   SET_FinalCombinerInputNV(disp, glFinalCombinerInputNV); */
/*   SET_GetCombinerInputParameterfvNV(disp, glGetCombinerInputParameterfvNV); */
/*   SET_GetCombinerInputParameterivNV(disp, glGetCombinerInputParameterivNV); */
/*   SET_GetCombinerOutputParameterfvNV(disp, glGetCombinerOutputParameterfvNV); */
/*   SET_GetCombinerOutputParameterivNV(disp, glGetCombinerOutputParameterivNV); */
/*   SET_GetFinalCombinerInputParameterfvNV(disp, glGetFinalCombinerInputParameterfvNV); */
/*   SET_GetFinalCombinerInputParameterivNV(disp, glGetFinalCombinerInputParameterivNV); */
#endif

#if GL_NV_vertex_array_range
/*   SET_FlushVertexArrayRangeNV(disp, glFlushVertexArrayRangeNV); */
/*   SET_VertexArrayRangeNV(disp, glVertexArrayRangeNV); */
#endif

#if GL_NV_vertex_program
/*   SET_AreProgramsResidentNV(disp, glAreProgramsResidentNV); */
/*   SET_BindProgramNV(disp, glBindProgramNV); */
/*   SET_DeleteProgramsNV(disp, glDeleteProgramsNV); */
/*   SET_ExecuteProgramNV(disp, glExecuteProgramNV); */
/*   SET_GenProgramsNV(disp, glGenProgramsNV); */
/*   SET_GetProgramParameterdvNV(disp, glGetProgramParameterdvNV); */
/*   SET_GetProgramParameterfvNV(disp, glGetProgramParameterfvNV); */
/*   SET_GetProgramStringNV(disp, glGetProgramStringNV); */
/*   SET_GetProgramivNV(disp, glGetProgramivNV); */
/*   SET_GetTrackMatrixivNV(disp, glGetTrackMatrixivNV); */
/*   SET_GetVertexAttribPointervNV(disp, glGetVertexAttribPointervNV); */
/*   SET_GetVertexAttribdvNV(disp, glGetVertexAttribdvNV); */
/*   SET_GetVertexAttribfvNV(disp, glGetVertexAttribfvNV); */
/*   SET_GetVertexAttribivNV(disp, glGetVertexAttribivNV); */
/*   SET_IsProgramNV(disp, glIsProgramNV); */
/*   SET_LoadProgramNV(disp, glLoadProgramNV); */
/*   SET_ProgramParameters4dvNV(disp, glProgramParameters4dvNV); */
/*   SET_ProgramParameters4fvNV(disp, glProgramParameters4fvNV); */
/*   SET_RequestResidentProgramsNV(disp, glRequestResidentProgramsNV); */
/*   SET_TrackMatrixNV(disp, glTrackMatrixNV); */
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
  SET_SampleMaskSGIS(disp, glSampleMaskSGIS);
  SET_SamplePatternSGIS(disp, glSamplePatternSGIS);
#endif

#if GL_SGIS_pixel_texture
/*   SET_GetPixelTexGenParameterfvSGIS(disp, glGetPixelTexGenParameterfvSGIS); */
/*   SET_GetPixelTexGenParameterivSGIS(disp, glGetPixelTexGenParameterivSGIS); */
/*   SET_PixelTexGenParameterfSGIS(disp, glPixelTexGenParameterfSGIS); */
/*   SET_PixelTexGenParameterfvSGIS(disp, glPixelTexGenParameterfvSGIS); */
/*   SET_PixelTexGenParameteriSGIS(disp, glPixelTexGenParameteriSGIS); */
/*   SET_PixelTexGenParameterivSGIS(disp, glPixelTexGenParameterivSGIS); */
/*   SET_PixelTexGenSGIX(disp, glPixelTexGenSGIX); */
#endif
}

