/*
 * File: glwindows.h
 * Purpose: Header for GLX implementation using native Windows OpenGL library
 *
 * Authors: Alexander Gottwald
 *          Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
 * Copyright (c) Alexander Gottwald 2004
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

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <glx/glxserver.h>
#include <glx/glxext.h>

#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <glx/glxscreens.h>
#include <glx/glxdrawable.h>
#include <glx/glxcontext.h>
#include <glx/glxutil.h>
#include <GL/internal/glcore.h>
#include <stdlib.h>


typedef struct {
    unsigned enableDebug : 1;
    unsigned enableTrace : 1;
    unsigned dumpPFD : 1;
    unsigned dumpHWND : 1;
    unsigned dumpDC : 1;
    unsigned enableGLcallTrace : 1;
} glWinDebugSettingsRec, *glWinDebugSettingsPtr;
extern glWinDebugSettingsRec glWinDebugSettings;

extern unsigned int glWinIndirectProcCalls;
extern unsigned int glWinDirectProcCalls;

void glWinCallDelta(void);
void setup_dispatch_table(void);
void glWinPushNativeProvider(void);
const GLubyte* glGetStringWrapperNonstatic(GLenum name);

#if 1
#define GLWIN_TRACE_MSG(msg, args...) if (glWinDebugSettings.enableTrace) ErrorF(msg " [%s:%d]\n" , ##args , __FUNCTION__, __LINE__ )
#define GLWIN_DEBUG_MSG(msg, args...) if (glWinDebugSettings.enableDebug) ErrorF(msg " [%s:%d]\n" , ##args , __FUNCTION__, __LINE__ )
#else
#define GLWIN_TRACE_MSG(a, ...)
#define GLWIN_DEBUG_MSG(a, ...)
#endif
