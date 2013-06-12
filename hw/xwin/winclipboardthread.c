/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2008
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the copyright holder(s)
 *and author(s) shall not be used in advertising or otherwise to promote
 *the sale, use or other dealings in this Software without prior written
 *authorization from the copyright holder(s) and author(s).
 *
 * Authors:	Harold L Hunt II
 *              Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#else
#define HAS_WINSOCK 1
#endif

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __CYGWIN__
#include <errno.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include "winclipboard.h"
#include "misc.h" // for ErrorF

// XXX: these should be passed into winClipboardProc() as arguments
extern void winSetAuthorization(void);
extern void winGetDisplayName(char *szDisplay, unsigned int screen);

/*
 * References to external symbols
 */

extern Bool g_fUnicodeClipboard;
extern Bool g_fClipboardStarted;
extern Bool g_fClipboardLaunched;
extern Bool g_fClipboard;
extern HWND g_hwndClipboard;
extern xcb_connection_t *g_pClipboardConn;
extern Window g_iClipboardWindow;

/*
 * Global variables
 */

int xfixes_event_base;

/*
 * Local function prototypes
 */

static void
winClipboardThreadExit(void *arg);

/*
 *
 */

static
xcb_screen_t *screen_of_display(xcb_connection_t *c,
                                 int screen)
{
  xcb_screen_iterator_t iter;
  iter = xcb_setup_roots_iterator(xcb_get_setup(c));
  for (; iter.rem; --screen, xcb_screen_next(&iter))
    if (screen == 0)
      return iter.data;

  return NULL;
}

void
assertSelectionOwnership(xcb_connection_t *c, xcb_window_t o, xcb_atom_t selection)
{
  {
    {
      xcb_void_cookie_t set_cookie = xcb_set_selection_owner_checked(c, selection, o, XCB_CURRENT_TIME);
      xcb_generic_error_t *error = xcb_request_check(c, set_cookie);

      if (error)
        {
          free(error);
          ErrorF("Could not set %d owner\n", selection);
          return;
        }
    }

    {
      xcb_window_t owner;
      xcb_generic_error_t *error;
      xcb_get_selection_owner_cookie_t get_cookie = xcb_get_selection_owner(c, selection);
      xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(c, get_cookie, &error);

      if (error) {
        ErrorF("Could not check owner of %d\n", selection);
        free(error);
        return;
      }

      owner = reply->owner;
      free(reply);

      if (owner != o) {
        ErrorF("Did not acquire %d ownership\n", selection);
        return;
      }
    }
    winDebug("Asserted ownership of %d\n", selection);
  }
}

/*
 * Main thread function
 */

void *
winClipboardProc(void *pvNotUsed)
{
    Atom atomClipboard;
    int iReturn;
    HWND hwnd = NULL;
    int fdConn = 0;

#ifdef HAS_DEVWINDOWS
    int fdMessageQueue = 0;
#else
    struct timeval tvTimeout;
#endif
    fd_set fdsRead;
    int iMaxDescriptor;
    int default_screen_no;
    xcb_connection_t *conn = NULL;
    Window iWindow = None;
    int iRetries;
    char szDisplay[512];
    int iSelectError;

    pthread_cleanup_push(&winClipboardThreadExit, NULL);

    winDebug("winClipboardProc - Hello\n");

    /* Use our generated cookie for authentication */
    winSetAuthorization();

    /* Initialize retry count */
    iRetries = 0;

    /* Setup the display connection string x */
    /*
     * NOTE: Always connect to screen 0 since we require that screen
     * numbers start at 0 and increase without gaps.  We only need
     * to connect to one screen on the display to get events
     * for all screens on the display.  That is why there is only
     * one clipboard client thread.
     */
    winGetDisplayName(szDisplay, 0);

    /* Print the display connection string */
    ErrorF("winClipboardProc - DISPLAY=%s\n", szDisplay);

    /* Open the X display */
    do {
        conn = xcb_connect(szDisplay, &default_screen_no);

        if (conn == NULL) {
            ErrorF("winClipboardProc - Could not open display, "
                   "try: %d, sleeping: %d\n", iRetries + 1, WIN_CONNECT_DELAY);
            ++iRetries;
            sleep(WIN_CONNECT_DELAY);
            continue;
        }
        else
            break;
    }
    while (conn == NULL && iRetries < WIN_CONNECT_RETRIES);

    /* Make sure that the display opened */
    if (conn == NULL) {
        ErrorF("winClipboardProc - Failed opening the display, giving up\n");
        goto winClipboardProc_Done;
    }

    /* Save the connection in a global used by the wndproc */
    // XXX: somehow pass this down to wndproc
    g_pClipboardConn = conn;

    ErrorF("winClipboardProc - Successfully opened the display.\n");

    /* Get our connection's fd */
    fdConn = xcb_get_file_descriptor(conn);

#ifdef HAS_DEVWINDOWS
    /* Open a file descriptor for the windows message queue */
    fdMessageQueue = open(WIN_MSG_QUEUE_FNAME, O_RDONLY);
    if (fdMessageQueue == -1) {
        ErrorF("winClipboardProc - Failed opening %s\n", WIN_MSG_QUEUE_FNAME);
        goto winClipboardProc_Done;
    }

    /* Find max of our file descriptors */
    iMaxDescriptor = max(fdMessageQueue, fdConn) + 1;
#else
    iMaxDescriptor = fdConn + 1;
#endif

    /* Check for XFIXES extension */
    {
      const char *extension_name = "XFIXES";
      xcb_query_extension_cookie_t cookie = xcb_query_extension(conn, strlen(extension_name), extension_name);
      xcb_query_extension_reply_t *reply = xcb_query_extension_reply(conn, cookie, NULL);

      if (!reply->present) {
        printf("%s extension not present, selection owner change detection will not work\n", extension_name);
      }
      else {
        xfixes_event_base = reply->first_event + XCB_XFIXES_SELECTION_EVENT_SET_SELECTION_OWNER;
      }
      free(reply);
    }

    /* Create atom */
    // XXX: write a utility function for this
    {
      const char *atom_name = "CLIPBOARD";
      xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
      xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
      if (reply)
        {
          atomClipboard = reply->atom;
          free(reply);
        }
    }

    /* Create a messaging window */
    iWindow = xcb_generate_id(conn);
    {
      xcb_screen_t *screen = screen_of_display(conn, default_screen_no);
      xcb_void_cookie_t cookie = xcb_create_window_checked(conn,
                                                           XCB_COPY_FROM_PARENT,
                                                           iWindow,
                                                           screen->root,
                                                           -1, -1, 1, 1, 0, /* 1x1 size, no border */
                                                           XCB_WINDOW_CLASS_INPUT_ONLY,
                                                           XCB_COPY_FROM_PARENT,
                                                           0, NULL);

      xcb_generic_error_t *e = xcb_request_check(conn, cookie);
      if (e) {
        ErrorF("winClipboardProc - Could not create an X window.\n");
        free(e);
        goto winClipboardProc_Done;
      }
    }

    // XXX: all xcb_ calls outside of event loop should be checked or have a reply so we note errors....
    {
      const char *title = "xwinclip";
      xcb_change_property(conn, XCB_PROP_MODE_REPLACE, iWindow, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    }

    /* Select event types to watch */
    {
      uint32_t values = { XCB_EVENT_MASK_PROPERTY_CHANGE };
      xcb_change_window_attributes(conn, iWindow, XCB_CW_EVENT_MASK, &values);
    }

    xcb_xfixes_select_selection_input(conn, iWindow, XA_PRIMARY,
    XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER | XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY | XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);

    xcb_xfixes_select_selection_input(conn, iWindow, atomClipboard,
    XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER | XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY | XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE);

    /* Save the window in the screen privates */
    // XXX: pass to wndproc somehow
    g_iClipboardWindow = iWindow;

    /* Create Windows messaging window */
    hwnd = winClipboardCreateMessagingWindow();

    /* Save copy of HWND in screen privates */
    g_hwndClipboard = hwnd;

    /* Assert ownership of PRIMARY and CLIPBOARD selections if Win32 clipboard is owned */
    if (NULL != GetClipboardOwner())
      {
        assertSelectionOwnership(conn, iWindow, XA_PRIMARY);
        assertSelectionOwnership(conn, iWindow, atomClipboard);
      }

    /* Pre-flush X events */
    /*
     * NOTE: Apparently you'll freeze if you don't do this,
     *       because there may be events in local data structures
     *       already.
     */
    winClipboardFlushXEvents(hwnd, iWindow, conn);

    /* Pre-flush Windows messages */
    if (!winClipboardFlushWindowsMessageQueue(hwnd)) {
        ErrorF("winClipboardProc - winClipboardFlushWindowsMessageQueue failed\n");
        pthread_exit(NULL);
    }

    /* Signal that the clipboard client has started */
    g_fClipboardStarted = TRUE;

    /* Loop for X events */
    while (1) {
        /* Setup the file descriptor set */
        /*
         * NOTE: You have to do this before every call to select
         *       because select modifies the mask to indicate
         *       which descriptors are ready.
         */
        FD_ZERO(&fdsRead);
        FD_SET(fdConn, &fdsRead);
#ifdef HAS_DEVWINDOWS
        FD_SET(fdMessageQueue, &fdsRead);
#else
        tvTimeout.tv_sec = 0;
        tvTimeout.tv_usec = 100;
#endif

        winDebug("winClipboardProc - Waiting in select\n");

        /* Wait for a Windows event or an X event */
        iReturn = select(iMaxDescriptor,        /* Highest fds number */
                         &fdsRead,      /* Read mask */
                         NULL,  /* No write mask */
                         NULL,  /* No exception mask */
#ifdef HAS_DEVWINDOWS
                         NULL   /* No timeout */
#else
                         &tvTimeout     /* Set timeout */
#endif
            );

#ifndef HAS_WINSOCK
        iSelectError = errno;
#else
        iSelectError = WSAGetLastError();
#endif

        if (iReturn < 0) {
#ifndef HAS_WINSOCK
            if (iSelectError == EINTR)
#else
            if (iSelectError == WSAEINTR)
#endif
                continue;

            ErrorF("winClipboardProc - Call to select () failed: %d.  "
                   "Bailing.\n", iReturn);
            break;
        }

        winDebug("winClipboardProc - select returned %d\n", iReturn);

        /* Branch on which descriptor became active */
        if (FD_ISSET(fdConn, &fdsRead)) {
            winDebug
                ("winClipboardProc - X connection ready, pumping X event queue\n");

            /* Process X events */
            /* Exit when we see that server is shutting down */
            iReturn = winClipboardFlushXEvents(hwnd, iWindow, conn);
            if (WIN_XEVENTS_SHUTDOWN == iReturn) {
                ErrorF("winClipboardProc - winClipboardFlushXEvents "
                       "trapped shutdown event, exiting main loop.\n");
                break;
            }
        }

#ifdef HAS_DEVWINDOWS
        /* Check for Windows event ready */
        if (FD_ISSET(fdMessageQueue, &fdsRead))
#else
        if (1)
#endif
        {
            winDebug
                ("winClipboardProc - /dev/windows ready, pumping Windows message queue\n");

            /* Process Windows messages */
            if (!winClipboardFlushWindowsMessageQueue(hwnd)) {
                ErrorF("winClipboardProc - "
                       "winClipboardFlushWindowsMessageQueue trapped "
                       "WM_QUIT message, exiting main loop.\n");
                break;
            }
        }

#ifdef HAS_DEVWINDOWS
        if (!(FD_ISSET(fdConn, &fdsRead)) &&
            !(FD_ISSET(fdMessageQueue, &fdsRead))) {
            winDebug("winClipboardProc - Spurious wake\n");
        }
#endif
    }

    /* disable the clipboard, which means the thread will die */
    g_fClipboard = FALSE;

 winClipboardProc_Done:
    /* Close our Windows window */
    if (g_hwndClipboard) {
        /* Destroy the Window window (hwnd) */
        winDebug("winClipboardProc - Destroy Windows window\n");
        PostMessage(g_hwndClipboard, WM_DESTROY, 0, 0);
        winClipboardFlushWindowsMessageQueue(g_hwndClipboard);
    }

    /* Close our X window */
    if (conn && iWindow) {
        xcb_void_cookie_t cookie = xcb_destroy_window_checked(conn, iWindow);
        xcb_generic_error_t *e = xcb_request_check(conn, cookie);

        if (e)
            ErrorF("winClipboardProc - XDestroyWindow failed.\n");
        else
            ErrorF("winClipboardProc - XDestroyWindow succeeded.\n");
    }

#ifdef HAS_DEVWINDOWS
    /* Close our Win32 message handle */
    if (fdMessageQueue)
        close(fdMessageQueue);
#endif

    /* Close our X display */
    if (conn) {
        xcb_disconnect(conn);
    }

    /* global clipboard variable reset */
    g_fClipboardLaunched = FALSE;
    g_fClipboardStarted = FALSE;
    g_iClipboardWindow = None;
    g_pClipboardConn = NULL;
    g_hwndClipboard = NULL;

    pthread_cleanup_pop(0);
    return NULL;
}

/*
 * winClipboardThreadExit - Thread exit handler
 */

static void
winClipboardThreadExit(void *arg)
{
    /* clipboard thread has exited, stop server as well */
    kill(getpid(), SIGTERM);
}
