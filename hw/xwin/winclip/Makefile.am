lib_LTLIBRARIES = libXWinclip.la

libXWinclip_la_SOURCES = \
	winclipboard.h \
	winclipboardtextconv.c \
	winclipboardthread.c \
	winclipboardwndproc.c \
	winclipboardxevents.c

AM_CFLAGS = -DHAVE_XWIN_CONFIG_H \
            $(DIX_CFLAGS) \
            $(XWINMODULES_CFLAGS) \
            -I$(top_srcdir) -I$(srcdir)/..

libXWinclip_la_LDFLAGS = -static -no-undefined

#bin_PROGRAMS = xwinclip
#
#xwinclip_SOURCES = xwinclip.c
#
#xwinclip_LDADD = libXWinclip.la $(XWIN_SYS_LIBS)
