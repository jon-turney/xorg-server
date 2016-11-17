lib_LTLIBRARIES = libXWinWMUtil.la

library_includedir=$(includedir)/XWinWMUtil

library_include_HEADERS = \
	icon_convert.h \
	cursor_convert.h \
	mouse.h \
	keyboard.h

libXWinWMUtil_la_SOURCES = \
	icon_convert.c \
	cursor_convert.c \
	mouse.c \
	keyboard.c \
	winvkmap.h \
	scancodes.h

AM_CFLAGS = -DHAVE_XWIN_CONFIG_H \
	    $(DIX_CFLAGS) \
	    $(XWINMODULES_CFLAGS) \
	    -I$(top_srcdir) -I$(top_srcdir)/miext/rootless -I$(srcdir)/..

libXWinWMUtil_la_LDFLAGS = -static -no-undefined
