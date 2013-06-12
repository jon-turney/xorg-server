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
#endif

#include <sys/types.h>
#include <sys/time.h>
#include "winclipboard.h"
#include "misc.h"
#include "winmsg.h"

extern void winFixClipboardChain(void);


/*
 * Constants
 */

#define WIN_POLL_TIMEOUT	1

/*
 * References to external symbols
 */

extern xcb_connection_t *g_pClipboardConn;
extern Window g_iClipboardWindow;

/*
 * Process X events up to specified timeout
 */

static int
winProcessXEventsTimeout(HWND hwnd, int iWindow, xcb_connection_t *conn, int iTimeoutSec)
{
  int fdConn;
  struct timeval tv;
  int iReturn;
  DWORD dwStopTime = GetTickCount() + iTimeoutSec * 1000;

  winDebug("winProcessXEventsTimeout () - pumping X events for %d seconds\n",
           iTimeoutSec);

  /* Get our connection number */
  fdConn = xcb_get_file_descriptor(conn);

  /* Loop for X events */
  while (1) {
    fd_set fdsRead;
    long remainingTime;

    /* We need to ensure that all pending events are processed */
    xcb_sync(conn);

    /* Setup the file descriptor set */
    FD_ZERO(&fdsRead);
    FD_SET(fdConn, &fdsRead);

    /* Adjust timeout */
    remainingTime = dwStopTime - GetTickCount();
    tv.tv_sec = remainingTime / 1000;
    tv.tv_usec = (remainingTime % 1000) * 1000;
    winDebug("winProcessXEventsTimeout () - %d milliseconds left\n",
             remainingTime);

    /* Break out if no time left */
    if (remainingTime <= 0)
      return WIN_XEVENTS_SUCCESS;

    /* Wait for an X event */
    iReturn = select(fdConn + 1,       /* Highest fds number */
                     &fdsRead,      /* Read mask */
                     NULL,  /* No write mask */
                     NULL,  /* No exception mask */
                     &tv);  /* Timeout */
    if (iReturn < 0) {
      ErrorF("winProcessXEventsTimeout - Call to select () failed: %d.  "
             "Bailing.\n", iReturn);
      break;
    }

    /* Branch on which descriptor became active */
    if (FD_ISSET(fdConn, &fdsRead)) {
      /* Process X events */
      /* Exit when we see that server is shutting down */
      iReturn = winClipboardFlushXEvents(hwnd, iWindow, conn);

      winDebug
        ("winProcessXEventsTimeout () - winClipboardFlushXEvents returned %d\n",
         iReturn);

      if (WIN_XEVENTS_NOTIFY == iReturn) {
        /* Bail out if notify processed */
        return iReturn;
      }
    }
    else {
      winDebug("winProcessXEventsTimeout - Spurious wake\n");
    }
  }

  return WIN_XEVENTS_SUCCESS;
}

/*
 * Process a given Windows message
 */

LRESULT CALLBACK
winClipboardWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND s_hwndNextViewer;
  static Bool s_fCBCInitialized;

#if CYGDEBUG
  winDebugWin32Message("winClipboardWindowProc", hwnd, message, wParam,
                       lParam);
#endif

  /* Branch on message type */
  switch (message) {
  case WM_DESTROY:
    {
      winDebug("winClipboardWindowProc - WM_DESTROY\n");

      /* Remove ourselves from the clipboard chain */
      ChangeClipboardChain(hwnd, s_hwndNextViewer);

      s_hwndNextViewer = NULL;

      PostQuitMessage(0);
    }
    return 0;

  case WM_CREATE:
    {
      HWND first, next;
      DWORD error_code = 0;

      winDebug("winClipboardWindowProc - WM_CREATE\n");

      first = GetClipboardViewer();   /* Get handle to first viewer in chain. */
      if (first == hwnd)
        return 0;           /* Make sure it's not us! */
      /* Add ourselves to the clipboard viewer chain */
      next = SetClipboardViewer(hwnd);
      error_code = GetLastError();
      if (SUCCEEDED(error_code) && (next == first))   /* SetClipboardViewer must have succeeded, and the handle */
        s_hwndNextViewer = next;    /* it returned must have been the first window in the chain */
      else
        s_fCBCInitialized = FALSE;
    }
    return 0;

  case WM_CHANGECBCHAIN:
    {
      winDebug("winClipboardWindowProc - WM_CHANGECBCHAIN: wParam(%x) "
               "lParam(%x) s_hwndNextViewer(%x)\n",
               wParam, lParam, s_hwndNextViewer);

      if ((HWND) wParam == s_hwndNextViewer) {
        s_hwndNextViewer = (HWND) lParam;
        if (s_hwndNextViewer == hwnd) {
          s_hwndNextViewer = NULL;
          winErrorFVerb(1, "winClipboardWindowProc - WM_CHANGECBCHAIN: "
                        "attempted to set next window to ourselves.");
        }
      }
      else if (s_hwndNextViewer)
        SendMessage(s_hwndNextViewer, message, wParam, lParam);

    }
    winDebug("winClipboardWindowProc - WM_CHANGECBCHAIN: Exit\n");
    return 0;

  case WM_WM_REINIT:
    {
      /* Ensure that we're in the clipboard chain.  Some apps,
       * WinXP's remote desktop for one, don't play nice with the
       * chain.  This message is called whenever we receive a
       * WM_ACTIVATEAPP message to ensure that we continue to
       * receive clipboard messages.
       *
       * It might be possible to detect if we're still in the chain
       * by calling SendMessage (GetClipboardViewer(),
       * WM_DRAWCLIPBOARD, 0, 0); and then seeing if we get the
       * WM_DRAWCLIPBOARD message.  That, however, might be more
       * expensive than just putting ourselves back into the chain.
       */

      HWND first, next;
      DWORD error_code = 0;

      winDebug("winClipboardWindowProc - WM_WM_REINIT: Enter\n");

      first = GetClipboardViewer();   /* Get handle to first viewer in chain. */
      if (first == hwnd)
        return 0;           /* Make sure it's not us! */
      winDebug("  WM_WM_REINIT: Replacing us(%x) with %x at head "
               "of chain\n", hwnd, s_hwndNextViewer);
      s_fCBCInitialized = FALSE;
      ChangeClipboardChain(hwnd, s_hwndNextViewer);
      s_hwndNextViewer = NULL;
      s_fCBCInitialized = FALSE;
      winDebug("  WM_WM_REINIT: Putting us back at head of chain.\n");
      first = GetClipboardViewer();   /* Get handle to first viewer in chain. */
      if (first == hwnd)
        return 0;           /* Make sure it's not us! */
      next = SetClipboardViewer(hwnd);
      error_code = GetLastError();
      if (SUCCEEDED(error_code) && (next == first))   /* SetClipboardViewer must have succeeded, and the handle */
        s_hwndNextViewer = next;    /* it returned must have been the first window in the chain */
      else
        s_fCBCInitialized = FALSE;
    }
    winDebug("winClipboardWindowProc - WM_WM_REINIT: Exit\n");
    return 0;

  case WM_DRAWCLIPBOARD:
    {
      static Atom atomClipboard;
      static int generation;
      static Bool s_fProcessingDrawClipboard = FALSE;
      xcb_connection_t *conn = g_pClipboardConn;
      Window iWindow = g_iClipboardWindow;

      winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Enter\n");

      if (generation != serverGeneration) {
        const char *atom_name = "CLIPBOARD";
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
        if (reply)
          {
            atomClipboard = reply->atom;
            free(reply);
          }

        generation = serverGeneration;
      }

      /*
       * We've occasionally seen a loop in the clipboard chain.
       * Try and fix it on the first hint of recursion.
       */
      if (!s_fProcessingDrawClipboard) {
        s_fProcessingDrawClipboard = TRUE;
      }
      else {
        /* Attempt to break the nesting by getting out of the chain, twice?, and then fix and bail */
        s_fCBCInitialized = FALSE;
        ChangeClipboardChain(hwnd, s_hwndNextViewer);
        winFixClipboardChain();
        winErrorFVerb(1, "winClipboardWindowProc - WM_DRAWCLIPBOARD - "
                      "Nested calls detected.  Re-initing.\n");
        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Exit\n");
        s_fProcessingDrawClipboard = FALSE;
        return 0;
      }

      /* Bail on first message */
      if (!s_fCBCInitialized) {
        s_fCBCInitialized = TRUE;
        s_fProcessingDrawClipboard = FALSE;
        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Exit\n");
        return 0;
      }

      /*
       * NOTE: We cannot bail out when NULL == GetClipboardOwner ()
       * because some applications deal with the clipboard in a manner
       * that causes the clipboard owner to be NULL when they are in
       * fact taking ownership.  One example of this is the Win32
       * native compile of emacs.
       */

      /* Bail when we still own the clipboard */
      if (hwnd == GetClipboardOwner()) {

        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD - "
                 "We own the clipboard, returning.\n");
        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Exit\n");
        s_fProcessingDrawClipboard = FALSE;
        if (s_hwndNextViewer)
          SendMessage(s_hwndNextViewer, message, wParam, lParam);
        return 0;
      }

      /*
       * Do not take ownership of the X11 selections when something
       * not convertible to CF_UNICODETEXT has been copied
       * into the Win32 clipboard.
       */
      if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {

        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD - "
                 "Clipboard does not contain CF_UNICODETEXT.\n");

        winDebug("winClipboardWindowProc: %d formats\n",
                 CountClipboardFormats());
        {
          unsigned int format = 0;

          do {
            format = EnumClipboardFormats(format);
            if (GetLastError() != ERROR_SUCCESS) {
              winDebug
                ("winClipboardWindowProc: EnumClipboardFormats failed %x\n",
                 GetLastError());
            }
            if (format > 0xc000) {
              char buff[256];

              GetClipboardFormatName(format, buff, 256);
              winDebug("winClipboardWindowProc: %d %s\n", format,
                       buff);
            }
            else if (format > 0)
              winDebug("winClipboardWindowProc: %d\n", format);
          } while (format != 0);
        }

        /*
         * We need to make sure that the X Server has processed
         * previous XSetSelectionOwner messages.
         */
        xcb_sync(conn);
        winDebug("winClipboardWindowProc - XSync done.\n");

        /* XXX: factor out common code */
        /* Release PRIMARY selection if owned */
        xcb_generic_error_t *error;
        xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(conn, XA_PRIMARY);
        xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(conn, cookie, &error);
        if (reply->owner == g_iClipboardWindow) {
          winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD - PRIMARY selection is owned by us, releasing\n");
          free(reply);
          xcb_set_selection_owner(conn, XCB_NONE, XA_PRIMARY, XCB_CURRENT_TIME);
        }
        else if (error)
          {
            winErrorFVerb(1, "winClipboardWindowProc - WM_DRAWCLIPBOARD - XGetSelectionOwner failed for PRIMARY: %d\n", error->error_code);
            free(error);
          }

        /* Release CLIPBOARD selection if owned */
        cookie = xcb_get_selection_owner(conn, atomClipboard);
        reply = xcb_get_selection_owner_reply(conn, cookie, &error);

        if (reply->owner == g_iClipboardWindow) {
          winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD - CLIPBOARD selection is owned by us, releasing\n");
          free(reply);

          xcb_set_selection_owner(conn, XCB_NONE, atomClipboard, XCB_CURRENT_TIME);
        }
        else if (error)
          {
            winErrorFVerb(1, "winClipboardWindowProc - WM_DRAWCLIPBOARD - XGetSelectionOwner failed for CLIPBOARD: %d\n", error->error_code);
            free(error);
          }

        winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Exit\n");
        s_fProcessingDrawClipboard = FALSE;
        if (s_hwndNextViewer)
          SendMessage(s_hwndNextViewer, message, wParam, lParam);
        return 0;
      }

      /* (Re)assert ownership of clipboard selections */
      assertSelectionOwnership(conn, iWindow, XA_PRIMARY);
      assertSelectionOwnership(conn, iWindow, atomClipboard);

      /* Flush any pending SetSelectionOwner event now */
      xcb_flush(conn);

      s_fProcessingDrawClipboard = FALSE;
    }
    winDebug("winClipboardWindowProc - WM_DRAWCLIPBOARD: Exit\n");
    /* Pass the message on the next window in the clipboard viewer chain */
    if (s_hwndNextViewer)
      SendMessage(s_hwndNextViewer, message, wParam, lParam);
    return 0;

  case WM_DESTROYCLIPBOARD:
    /*
     * NOTE: Intentionally do nothing.
     * Changes in the Win32 clipboard are handled by WM_DRAWCLIPBOARD
     * above.  We only process this message to conform to the specs
     * for delayed clipboard rendering in Win32.  You might think
     * that we need to release ownership of the X11 selections, but
     * we do not, because a WM_DRAWCLIPBOARD message will closely
     * follow this message and reassert ownership of the X11
     * selections, handling the issue for us.
     */
    winDebug("winClipboardWindowProc - WM_DESTROYCLIPBOARD - Ignored.\n");
    return 0;

  case WM_RENDERFORMAT:
  case WM_RENDERALLFORMATS:
    {
      int iReturn;
      xcb_connection_t *conn = g_pClipboardConn;
      Window iWindow = g_iClipboardWindow;
      xcb_atom_t atomLocalProperty;
      xcb_atom_t atomCompoundText;

      if (message == WM_RENDERALLFORMATS)
        winDebug("winClipboardWindowProc - WM_RENDERALLFORMATS - Hello.\n");
      else
        winDebug("winClipboardWindowProc - WM_RENDERFORMAT %d - Hello.\n",
                 wParam);

      {
        const char *atom_name = "COMPOUND_TEXT";
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
        if (reply)
          {
            atomCompoundText = reply->atom;
            free(reply);
          }
      }

      {
        const char *atom_name = WIN_LOCAL_PROPERTY;
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
        if (reply)
          {
            atomLocalProperty = reply->atom;
            free(reply);
          }
      }

      /* Request the selection contents */
      // XXX: why are requesting conversion to COMPOUND_TEXT and not UTF8-TEXT???
      xcb_void_cookie_t cookie = xcb_convert_selection_checked (conn, iWindow, GetLastOwnedSelectionAtom(), atomCompoundText, atomLocalProperty, XCB_CURRENT_TIME);
      xcb_generic_error_t *error = xcb_request_check(conn, cookie);
      if (error) {
        winErrorFVerb(1, "winClipboardWindowProc - WM_RENDER*FORMAT - xcb_convert_selection() failed\n");
        free(error);
        break;
      }

      /* Special handling for WM_RENDERALLFORMATS */
      if (message == WM_RENDERALLFORMATS) {
        /* We must open and empty the clipboard */

        /* Close clipboard if we have it open already */
        if (GetOpenClipboardWindow() == hwnd) {
          CloseClipboard();
        }

        if (!OpenClipboard(hwnd)) {
          winErrorFVerb(1, "winClipboardWindowProc - WM_RENDER*FORMATS - OpenClipboard () failed: %08x\n", GetLastError());
          break;
        }

        if (!EmptyClipboard()) {
          winErrorFVerb(1, "winClipboardWindowProc - WM_RENDER*FORMATS - EmptyClipboard () failed: %08x\n", GetLastError());
          break;
        }
      }

      /* Process the SelectionNotify event */
      iReturn = winProcessXEventsTimeout(hwnd, iWindow, conn, WIN_POLL_TIMEOUT);

      /*
       * The last call to winProcessXEventsTimeout
       * from above had better have seen a notify event, or else we
       * are dealing with a buggy or old X11 app.  In these cases we
       * have to paste some fake data to the Win32 clipboard to
       * satisfy the requirement that we write something to it.
       */
      if (WIN_XEVENTS_NOTIFY != iReturn) {
        /* Paste no data, to satisfy required call to SetClipboardData */
        SetClipboardData(CF_UNICODETEXT, NULL);
        SetClipboardData(CF_TEXT, NULL);

        ErrorF("winClipboardWindowProc - timed out waiting for WIN_XEVENTS_NOTIFY\n");
      }

      /* Special handling for WM_RENDERALLFORMATS */
      if (message == WM_RENDERALLFORMATS) {
        /* We must close the clipboard */
        if (!CloseClipboard()) {
          winErrorFVerb(1, "winClipboardWindowProc - WM_RENDERALLFORMATS - CloseClipboard () failed: %08x\n", GetLastError());
          break;
        }
      }

      winDebug("winClipboardWindowProc - WM_RENDER*FORMAT - Returning.\n");
      return 0;
    }
  }

  /* Let Windows perform default processing for unhandled messages */
  return DefWindowProc(hwnd, message, wParam, lParam);
}

/*
 * Process any pending Windows messages
 */

BOOL
winClipboardFlushWindowsMessageQueue(HWND hwnd)
{
  MSG msg;

  /* Flush the messaging window queue */
  /* NOTE: Do not pass the hwnd of our messaging window to PeekMessage,
   * as this will filter out many non-window-specific messages that
   * are sent to our thread, such as WM_QUIT.
   */
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    /* Dispatch the message if not WM_QUIT */
    if (msg.message == WM_QUIT)
      return FALSE;
    else
      DispatchMessage(&msg);
  }

  return TRUE;
}
