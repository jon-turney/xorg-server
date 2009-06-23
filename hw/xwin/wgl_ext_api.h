/*
 * File: wgl_ext_api.h
 * Purpose: Wrapper functions for Win32 OpenGL wgl extension functions
 *
 * Authors: Jon TURNEY
 *
 * Copyright (c) Jon TURNEY 2009
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

#ifndef wgl_ext_api_h
#define wgl_ext_api_h

void wglResolveExtensionProcs(void);

const char *wglGetExtensionsStringARBWrapper(HDC hdc);
wBOOL wglMakeContextCurrentARBWrapper(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
HDC wglGetCurrentReadDCARBWrapper(VOID);

wBOOL wglGetPixelFormatAttribivARBWrapper(HDC hdc,
                                          int iPixelFormat,
                                          int iLayerPlane,
                                          UINT nAttributes,
                                          const int *piAttributes,
                                          int *piValues);

wBOOL wglGetPixelFormatAttribfvARBWrapper(HDC hdc,
                                          int iPixelFormat,
                                          int iLayerPlane,
                                          UINT nAttributes,
                                          const int *piAttributes,
                                          FLOAT *pfValues);

wBOOL wglChoosePixelFormatARBWrapper(HDC hdc,
                                     const int *piAttribIList,
                                     const FLOAT *pfAttribFList,
                                     UINT nMaxFormats,
                                     int *piFormats,
                                     UINT *nNumFormats);

DECLARE_HANDLE(HPBUFFERARB);

HPBUFFERARB wglCreatePbufferARBWrapper(HDC hDC,
                                       int iPixelFormat,
                                       int iWidth,
                                       int iHeight,
                                       const int *piAttribList);

HDC wglGetPbufferDCARBWrapper(HPBUFFERARB hPbuffer);

int wglReleasePbufferDCARBWrapper(HPBUFFERARB hPbuffer,
                                  HDC hDC);

wBOOL wglDestroyPbufferARBWrapper(HPBUFFERARB hPbuffer);

wBOOL wglQueryPbufferARBWrapper(HPBUFFERARB hPbuffer,
                                int iAttribute,
                                int *piValue);

wBOOL wglSwapIntervalEXTWrapper(int interval);

int wglGetSwapIntervalEXTWrapper(void);



#define WGL_NUMBER_PIXEL_FORMATS_ARB            0x2000
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_DRAW_TO_BITMAP_ARB                  0x2002
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_NEED_PALETTE_ARB                    0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB             0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB              0x2006
#define WGL_SWAP_METHOD_ARB                     0x2007
#define WGL_NUMBER_OVERLAYS_ARB                 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                0x2009
#define WGL_TRANSPARENT_ARB                     0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB           0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB         0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB          0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB         0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB         0x203B
#define WGL_SHARE_DEPTH_ARB                     0x200C
#define WGL_SHARE_STENCIL_ARB                   0x200D
#define WGL_SHARE_ACCUM_ARB                     0x200E
#define WGL_SUPPORT_GDI_ARB                     0x200F
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_STEREO_ARB                          0x2012
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_RED_SHIFT_ARB                       0x2016
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_GREEN_SHIFT_ARB                     0x2018
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_BLUE_SHIFT_ARB                      0x201A
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_ALPHA_SHIFT_ARB                     0x201C
#define WGL_ACCUM_BITS_ARB                      0x201D
#define WGL_ACCUM_RED_BITS_ARB                  0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                0x2021
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_AUX_BUFFERS_ARB                     0x2024

#define WGL_NO_ACCELERATION_ARB                 0x2025
#define WGL_GENERIC_ACCELERATION_ARB            0x2026
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_SWAP_EXCHANGE_ARB                   0x2028
#define WGL_SWAP_COPY_ARB                       0x2029
#define WGL_SWAP_UNDEFINED_ARB                  0x202A

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_TYPE_COLORINDEX_ARB                 0x202C

// WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB                  0x2041
#define WGL_SAMPLES_ARB                         0x2042

// WGL_ARB_render_texture
#define WGL_BIND_TO_TEXTURE_RGB_ARB             0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB            0x2071

// WGL_ARB_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ARB                 0x21A0

// WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB                 0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB              0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB               0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB              0x2030

#endif /* wgl_ext_api_h */
