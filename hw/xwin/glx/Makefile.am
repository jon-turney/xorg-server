noinst_LTLIBRARIES = libXwinGLX.la

libXwinGLX_la_SOURCES = \
	winpriv.c \
	winpriv.h \
	glwindows.h \
	glwrap.c \
	indirect.c \
	wgl_ext_api.c \
	wgl_ext_api.h

if XWIN_MULTIWINDOW
DEFS_MULTIWINDOW = -DXWIN_MULTIWINDOW
endif

if XWIN_MULTIWINDOWEXTWM
DEFS_MULTIWINDOWEXTWM = -DXWIN_MULTIWINDOWEXTWM
endif

if XWIN_GLX_WINDOWS
DEFS_GLX_WINDOWS = -DXWIN_GLX_WINDOWS
endif

DEFS =  $(DEFS_MULTIWINDOW) $(DEFS_MULTIWINDOWEXTWM) $(DEFS_GLX_WINDOWS)

INCLUDES = -I$(top_srcdir)/miext/rootless

AM_CFLAGS = -DHAVE_XWIN_CONFIG_H $(DIX_CFLAGS) \
            $(XWINMODULES_CFLAGS) \
            -I$(top_srcdir) \
            -I$(top_srcdir)/hw/xwin/

if XWIN_GLX_WINDOWS

PYTHONPATH := $(PYTHONPATH):$(KHRONOS_SPEC_DIR)

generated_gl_wrappers.c: gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/gl.xml $(KHRONOS_SPEC_DIR)/reg.py
	echo $(PYTHONPATH)
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/gl.xml -dispatchheader $(top_srcdir)/glx/dispatch.h -staticwrappers -outfile $@

generated_wgl_wrappers.c: gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/wgl.xml $(KHRONOS_SPEC_DIR)/reg.py
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/wgl.xml -prefix wgl -preresolve -outfile $@
endif

BUILT_SOURCES = generated_gl_wrappers.c generated_wgl_wrappers.c
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gen_gl_wrappers.py
