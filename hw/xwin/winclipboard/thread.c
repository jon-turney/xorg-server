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
#ifdef __CYGWIN__
#include <errno.h>
#endif

#include "misc.h"
#include <X11/extensions/Xfixes.h>
#include "winclipboard.h"
#include "internal.h"

#define WIN_CONNECT_RETRIES			40
#define WIN_CONNECT_DELAY			4

#define WIN_CLIPBOARD_WINDOW_CLASS		"xwinclip"
#define WIN_CLIPBOARD_WINDOW_TITLE		"xwinclip"

/*
 * Global variables
 */

static HWND g_hwndClipboard = NULL;
static jmp_buf g_jmpEntry;
static XIOErrorHandler g_winClipboardOldIOErrorHandler;
static pthread_t g_winClipboardProcThread;

int xfixes_event_base;
int xfixes_error_base;

/*
 * Local function prototypes
 */

static HWND
winClipboardCreateMessagingWindow(Display *pDisplay, Window iWindow);

static int
 winClipboardErrorHandler(Display * pDisplay, XErrorEvent * pErr);

static int
 winClipboardIOErrorHandler(Display * pDisplay);

/*
 * Create X11 and Win32 messaging windows, and run message processing loop
 *
 * returns TRUE if shutdown was signalled to loop, FALSE if some error occurred
 */

Bool
winClipboardProc(Bool fUseUnicode, char *szDisplay)
{
    Atom atomClipboard;
    int iReturn;
    HWND hwnd = NULL;
    int iConnectionNumber = 0;

#ifdef HAS_DEVWINDOWS
    int fdMessageQueue = 0;
#else
    struct timeval tvTimeout;
#endif
    fd_set fdsRead;
    int iMaxDescriptor;
    Display *pDisplay = NULL;
    Window iWindow = None;
    int iRetries;
    int iSelectError;
    Bool fShutdown = FALSE;

    winDebug("winClipboardProc - Hello\n");

    /* Set error handler */
    XSetErrorHandler(winClipboardErrorHandler);
    g_winClipboardProcThread = pthread_self();
    g_winClipboardOldIOErrorHandler =
        XSetIOErrorHandler(winClipboardIOErrorHandler);

    /* Set jump point for Error exits */
    iReturn = setjmp(g_jmpEntry);

    /* Check if we should continue operations */
    if (iReturn != WIN_JMP_ERROR_IO && iReturn != WIN_JMP_OKAY) {
        /* setjmp returned an unknown value, exit */
        ErrorF("winClipboardProc - setjmp returned: %d exiting\n", iReturn);
        goto winClipboardProc_Exit;
    }
    else if (iReturn == WIN_JMP_ERROR_IO) {
        /* TODO: Cleanup the Win32 window and free any allocated memory */
        ErrorF("winClipboardProc - setjmp returned for IO Error Handler.\n");
        goto winClipboardProc_Done;
    }

    /* Initialize retry count */
    iRetries = 0;

    /* Open the X display */
    do {
        pDisplay = XOpenDisplay(szDisplay);
        if (pDisplay == NULL) {
            ErrorF("winClipboardProc - Could not open display, "
                   "try: %d, sleeping: %d\n", iRetries + 1, WIN_CONNECT_DELAY);
            ++iRetries;
            sleep(WIN_CONNECT_DELAY);
            continue;
        }
        else
            break;
    }
    while (pDisplay == NULL && iRetries < WIN_CONNECT_RETRIES);

    /* Make sure that the display opened */
    if (pDisplay == NULL) {
        ErrorF("winClipboardProc - Failed opening the display, giving up\n");
        goto winClipboardProc_Done;
    }

    ErrorF("winClipboardProc - XOpenDisplay () returned and "
           "successfully opened the display.\n");

    /* Get our connection number */
    iConnectionNumber = XConnectionNumber(pDisplay);

#ifdef HAS_DEVWINDOWS
    /* Open a file descriptor for the windows message queue */
    fdMessageQueue = open(WIN_MSG_QUEUE_FNAME, O_RDONLY);
    if (fdMessageQueue == -1) {
        ErrorF("winClipboardProc - Failed opening %s\n", WIN_MSG_QUEUE_FNAME);
        goto winClipboardProc_Done;
    }

    /* Find max of our file descriptors */
    iMaxDescriptor = max(fdMessageQueue, iConnectionNumber) + 1;
#else
    iMaxDescriptor = iConnectionNumber + 1;
#endif

    if (!XFixesQueryExtension(pDisplay, &xfixes_event_base, &xfixes_error_base))
      ErrorF ("winClipboardProc - XFixes extension not present\n");

    /* Create atom */
    atomClipboard = XInternAtom(pDisplay, "CLIPBOARD", False);

    /* Create a messaging window */
    iWindow = XCreateSimpleWindow(pDisplay,
                                  XDefaultRootWindow(pDisplay),
                                  1, 1,
                                  500, 500,
                                  0,
                                  XBlackPixel(pDisplay, 0),
                                  XBlackPixel(pDisplay, 0));
    if (iWindow == 0) {
        ErrorF("winClipboardProc - Could not create an X window.\n");
        goto winClipboardProc_Done;
    }

    XStoreName(pDisplay, iWindow, "xwinclip");

    /* Select event types to watch */
    if (XSelectInput(pDisplay, iWindow, PropertyChangeMask) == BadWindow)
        ErrorF("winClipboardProc - XSelectInput generated BadWindow "
               "on messaging window\n");

    XFixesSelectSelectionInput (pDisplay,
                                iWindow,
                                XA_PRIMARY,
                                XFixesSetSelectionOwnerNotifyMask |
                                XFixesSelectionWindowDestroyNotifyMask |
                                XFixesSelectionClientCloseNotifyMask);

    XFixesSelectSelectionInput (pDisplay,
                                iWindow,
                                XInternAtom (pDisplay, "CLIPBOARD", False),
                                XFixesSetSelectionOwnerNotifyMask |
                                XFixesSelectionWindowDestroyNotifyMask |
                                XFixesSelectionClientCloseNotifyMask);


    /* Initialize monitored selection state */
    winClipboardInitMonitoredSelections();
    /* Create Windows messaging window */
    hwnd = winClipboardCreateMessagingWindow(pDisplay, iWindow);

    /* Save copy of HWND */
    g_hwndClipboard = hwnd;

    /* Assert ownership of selections if Win32 clipboard is owned */
    if (NULL != GetClipboardOwner()) {
        /* PRIMARY */
        iReturn = XSetSelectionOwner(pDisplay, XA_PRIMARY,
                                     iWindow, CurrentTime);
        if (iReturn == BadAtom || iReturn == BadWindow ||
            XGetSelectionOwner(pDisplay, XA_PRIMARY) != iWindow) {
            ErrorF("winClipboardProc - Could not set PRIMARY owner\n");
            goto winClipboardProc_Done;
        }

        /* CLIPBOARD */
        iReturn = XSetSelectionOwner(pDisplay, atomClipboard,
                                     iWindow, CurrentTime);
        if (iReturn == BadAtom || iReturn == BadWindow ||
            XGetSelectionOwner(pDisplay, atomClipboard) != iWindow) {
            ErrorF("winClipboardProc - Could not set CLIPBOARD owner\n");
            goto winClipboardProc_Done;
        }
    }

    /* Pre-flush X events */
    /*
     * NOTE: Apparently you'll freeze if you don't do this,
     *       because there may be events in local data structures
     *       already.
     */
    winClipboardFlushXEvents(hwnd, iWindow, pDisplay, fUseUnicode);

    /* Pre-flush Windows messages */
    if (!winClipboardFlushWindowsMessageQueue(hwnd)) {
        ErrorF("winClipboardProc - winClipboardFlushWindowsMessageQueue failed\n");
        pthread_exit(NULL);
    }

    /* Loop for X events */
    while (1) {
        /* Setup the file descriptor set */
        /*
         * NOTE: You have to do this before every call to select
         *       because select modifies the mask to indicate
         *       which descriptors are ready.
         */
        FD_ZERO(&fdsRead);
        FD_SET(iConnectionNumber, &fdsRead);
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
        if (FD_ISSET(iConnectionNumber, &fdsRead)) {
            winDebug
                ("winClipboardProc - X connection ready, pumping X event queue\n");

            /* Process X events */
            winClipboardFlushXEvents(hwnd, iWindow, pDisplay, fUseUnicode);
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
        if (!(FD_ISSET(iConnectionNumber, &fdsRead)) &&
            !(FD_ISSET(fdMessageQueue, &fdsRead))) {
            winDebug("winClipboardProc - Spurious wake\n");
        }
#endif
    }

 winClipboardProc_Exit:
    /* broke out of while loop on a shutdown message */
    fShutdown = TRUE;

 winClipboardProc_Done:
    /* Close our Windows window */
    if (g_hwndClipboard) {
        winClipboardWindowDestroy();
    }

    /* Close our X window */
    if (pDisplay && iWindow) {
        iReturn = XDestroyWindow(pDisplay, iWindow);
        if (iReturn == BadWindow)
            ErrorF("winClipboardProc - XDestroyWindow returned BadWindow.\n");
        else
            ErrorF("winClipboardProc - XDestroyWindow succeeded.\n");
    }

#ifdef HAS_DEVWINDOWS
    /* Close our Win32 message handle */
    if (fdMessageQueue)
        close(fdMessageQueue);
#endif

#if 0
    /*
     * FIXME: XCloseDisplay hangs if we call it, as of 2004/03/26.  The
     * XSync and XSelectInput calls did not help.
     */

    /* Discard any remaining events */
    XSync(pDisplay, TRUE);

    /* Select event types to watch */
    XSelectInput(pDisplay, XDefaultRootWindow(pDisplay), None);

    /* Close our X display */
    if (pDisplay) {
        XCloseDisplay(pDisplay);
    }
#endif

    /* global clipboard variable reset */
    g_hwndClipboard = NULL;

    return fShutdown;
}

/*
 * Create the Windows window that we use to receive Windows messages
 */

static HWND
winClipboardCreateMessagingWindow(Display *pDisplay, Window iWindow)
{
    WNDCLASSEX wc;
    HWND hwnd;

    /* Setup our window class */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = winClipboardWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WIN_CLIPBOARD_WINDOW_CLASS;
    wc.hIconSm = 0;
    RegisterClassEx(&wc);

    /* Information to be passed to WM_CREATE */
    ClipboardWindowCreationParams cwcp;
    cwcp.pClipboardDisplay = pDisplay;
    cwcp.iClipboardWindow = iWindow;

    /* Create the window */
    hwnd = CreateWindowExA(0,   /* Extended styles */
                           WIN_CLIPBOARD_WINDOW_CLASS,  /* Class name */
                           WIN_CLIPBOARD_WINDOW_TITLE,  /* Window name */
                           WS_OVERLAPPED,       /* Not visible anyway */
                           CW_USEDEFAULT,       /* Horizontal position */
                           CW_USEDEFAULT,       /* Vertical position */
                           CW_USEDEFAULT,       /* Right edge */
                           CW_USEDEFAULT,       /* Bottom edge */
                           (HWND) NULL, /* No parent or owner window */
                           (HMENU) NULL,        /* No menu */
                           GetModuleHandle(NULL),       /* Instance handle */
                           &cwcp);       /* Creation data */
    assert(hwnd != NULL);

    /* I'm not sure, but we may need to call this to start message processing */
    ShowWindow(hwnd, SW_HIDE);

    /* Similarly, we may need a call to this even though we don't paint */
    UpdateWindow(hwnd);

    return hwnd;
}

/*
 * winClipboardErrorHandler - Our application specific error handler
 */

static int
winClipboardErrorHandler(Display * pDisplay, XErrorEvent * pErr)
{
    char pszErrorMsg[100];

    XGetErrorText(pDisplay, pErr->error_code, pszErrorMsg, sizeof(pszErrorMsg));
    ErrorF("winClipboardErrorHandler - ERROR: \n\t%s\n"
           "\tSerial: %lu, Request Code: %d, Minor Code: %d\n",
           pszErrorMsg, pErr->serial, pErr->request_code, pErr->minor_code);
    return 0;
}

/*
 * winClipboardIOErrorHandler - Our application specific IO error handler
 */

static int
winClipboardIOErrorHandler(Display * pDisplay)
{
    ErrorF("winClipboardIOErrorHandler!\n");

    if (pthread_equal(pthread_self(), g_winClipboardProcThread)) {
        /* Restart at the main entry point */
        longjmp(g_jmpEntry, WIN_JMP_ERROR_IO);
    }

    if (g_winClipboardOldIOErrorHandler)
        g_winClipboardOldIOErrorHandler(pDisplay);

    return 0;
}

void
winClipboardWindowDestroy(void)
{
  if (g_hwndClipboard) {
    SendMessage(g_hwndClipboard, WM_DESTROY, 0, 0);
  }
}

void
winFixClipboardChain(void)
{
  if (g_hwndClipboard) {
    PostMessage(g_hwndClipboard, WM_WM_REINIT, 0, 0);
  }
}
