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

#include <iconv.h>

#include <locale.h>

#include "winclipboard.h"
#include "misc.h" //for ServerGeneration

#include <xcb/xfixes.h>

/*
 * Constants
 */

#define CLIP_NUM_SELECTIONS		2
#define CLIP_OWN_NONE     		-1
#define CLIP_OWN_PRIMARY		0
#define CLIP_OWN_CLIPBOARD		1

/*
 * Global variables
 */

extern int xfixes_event_base;

/*
 * Local variables
 */

static xcb_window_t s_iOwners[CLIP_NUM_SELECTIONS] = {None, None};
static const char *szSelectionNames[CLIP_NUM_SELECTIONS] = { "PRIMARY", "CLIPBOARD"};

static unsigned int lastOwnedSelectionIndex = CLIP_OWN_NONE;
static xcb_atom_t atomClipboard = XCB_NONE;
static xcb_atom_t atomCompoundText = XCB_NONE;
static xcb_atom_t atomUTF8String = XCB_NONE;

static void
MonitorSelection(xcb_xfixes_selection_notify_event_t *e, unsigned int i)
{
  /* Look for owned -> not owned transition */
  if (XCB_NONE == e->owner && XCB_NONE != s_iOwners[i])
    {
      unsigned int other_index;
      winDebug("MonitorSelection - %s - Going from owned to not owned.\n", szSelectionNames[i]);

      /* If this selection is not owned, the other monitored selection must be the most
         recently owned, if it is owned at all */
      if (i == CLIP_OWN_PRIMARY) other_index = CLIP_OWN_CLIPBOARD;
      if (i == CLIP_OWN_CLIPBOARD) other_index = CLIP_OWN_PRIMARY;
      if (None != s_iOwners[other_index])
        lastOwnedSelectionIndex = other_index;
      else
        lastOwnedSelectionIndex = CLIP_OWN_NONE;
    }

  /* Save new selection owner or None */
  s_iOwners[i] = e->owner;
  winDebug("MonitorSelection - %s - Now owned by XID %x\n", szSelectionNames[i], e->owner);
}

xcb_atom_t
GetLastOwnedSelectionAtom(void)
{
  if (lastOwnedSelectionIndex == CLIP_OWN_NONE)
    return None;

  if (lastOwnedSelectionIndex == CLIP_OWN_PRIMARY)
    return XA_PRIMARY;

  if (lastOwnedSelectionIndex == CLIP_OWN_CLIPBOARD)
    return atomClipboard;

  return None;
}

static
const char *encodingNameFromAtom(xcb_atom_t atom)
{
  if (atom == XA_STRING)
    return "ISO-8859-1//TRANSLIT";
  else if (atom == atomUTF8String)
    return "UTF-8";
  else if (atom == atomCompoundText)
    return "ct"; // probably not supported by GNU iconv :D

  ErrorF("Unknown target encoding, treating as Latin1\n");
  return "ISO-8859-1";
}

static
char *convertText(const char *tocode, const char *fromcode, char *data, size_t datalen, size_t *outlen)
{
  iconv_t cd = iconv_open(tocode, fromcode);
  if (cd == (iconv_t)-1)
    {
      ErrorF("iconv conversion not supported");
      return NULL;
    }

  char *inbuf = data;
  size_t outbuflen = datalen * 2; // XXX: pathological encodings could be more than this...???
  char *outbuf = malloc(outbuflen);

  size_t inbytesleft = datalen;
  size_t outbytesleft = outbuflen;
  size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  iconv_close(cd);

  if (result < 0) {
    ErrorF("iconv conversion failed: %d\n", errno);
    return NULL;
  }

  *outlen = outbuflen - outbytesleft;
  return outbuf;
}

/*
 * Process any pending X events
 */

int
winClipboardFlushXEvents(HWND hwnd, xcb_window_t iWindow, xcb_connection_t *conn)
{
  static xcb_atom_t atomLocalProperty;
  static xcb_atom_t atomTargets;
  static int generation;

  if (generation != serverGeneration) {
    struct
    {
      const char *name;
      xcb_atom_t *atom;
    } names[] =
        {
          { "CLIPBOARD", &atomClipboard },
          { WIN_LOCAL_PROPERTY, &atomLocalProperty },
          { "UTF8_STRING", &atomUTF8String },
          { "COMPOUND_TEXT", &atomCompoundText },
          { "TARGETS", &atomTargets },
        };

#define NUM_NAMES (sizeof((names)) / sizeof((names)[0]))

    xcb_intern_atom_cookie_t cookies[NUM_NAMES];

    for (int i = 0; i < NUM_NAMES; ++i) {
      cookies[i] = xcb_intern_atom(conn, 0, strlen(names[i].name), names[i].name);
    }

    for (int i = 0; i < NUM_NAMES; ++i) {
      xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply (conn, cookies[i], NULL ); // normally a pointer to receive errors, but we'll just ignore error handling
      if (reply) {
        *(names[i].atom) = reply->atom;
        free(reply);
      }
    }

    /* Initialize static variables */
    for (int i = 0; i < CLIP_NUM_SELECTIONS; ++i)
      s_iOwners[i] = None;

    generation = serverGeneration;
  }

  /* Process all pending events */
  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(conn))) {
    char *pszGlobalData = NULL;
    HGLOBAL hGlobal = NULL;
    int iConvertDataLen = 0;
    char *pszConvertData = NULL;
    wchar_t *pwszUnicodeStr = NULL;
    int iUnicodeLen = 0;
    Bool fAbort = FALSE;
    Bool fCloseClipboard = FALSE;
    Bool fSetClipboardData = TRUE;

    /* Branch on the event type */
    switch (event->response_type & ~0x80) {

      /*
       * SelectionRequest
       */

    case XCB_SELECTION_REQUEST:
      {
        xcb_selection_request_event_t *e = (xcb_selection_request_event_t *)event;

        // XXX: this should be factored out as a helper function
        {
          xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, e->target);
          xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);

          winDebug("SelectionRequest - Target %d\n", e->target);
          winDebug("SelectionRequest - Target atom name %s\n", reply ? xcb_get_atom_name_name(reply) : "unknown");
          free(reply);
        }

        /* Abort if invalid target type */
        if (e->target != XA_STRING
            && e->target != atomUTF8String
            && e->target != atomCompoundText
            && e->target != atomTargets) {
          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

        /* Handle "TARGETS" */
        if (e->target == atomTargets) {
          xcb_atom_t atomTargetArr[] = { atomTargets,
                                   atomCompoundText,
                                   atomUTF8String,
                                   XA_STRING
          };

          winDebug("SelectionRequest - populating targets\n");

          /* Try to change the property */
          xcb_void_cookie_t cookie = xcb_change_property_checked(conn, XCB_PROP_MODE_REPLACE,
                                                                 e->requestor,
                                                                 e->property,
                                                                 XA_ATOM,
                                                                 32,
                                                                 (sizeof(atomTargetArr) / sizeof(atomTargetArr[0])),
                                                                 (unsigned char *) atomTargetArr);

          xcb_generic_error_t *error = xcb_request_check(conn, cookie);
          if (error) {
            ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                   "XChangeProperty failed: %d\n", error->error_code);
            free(error);
          }

          /* Setup selection notify xevent */
          xcb_selection_notify_event_t eventSelection;
          eventSelection.response_type = SelectionNotify;
          eventSelection.requestor = e->requestor;
          eventSelection.selection = e->selection;
          eventSelection.target = e->target;
          eventSelection.property = e->property;
          eventSelection.time = e->time;

          /*
           * Notify the requesting window that
           * the operation has completed
           */
          cookie = xcb_send_event_checked(conn, FALSE, eventSelection.requestor,
                                          0, (const char *)&eventSelection);

          error = xcb_request_check(conn, cookie);

          if (error) {
            ErrorF("winClipboardFlushXEvents - SelectionRequest - send event failed\n");
          }

          /* done processing for "TARGETS" request */
          break;
        }

        /* Close clipboard if we have it open already */
        if (GetOpenClipboardWindow() == hwnd) {
          CloseClipboard();
        }

        /* Access the clipboard */
        if (!OpenClipboard(hwnd)) {
          ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                 "OpenClipboard () failed: %08lx\n", GetLastError());

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

        /* Indicate that clipboard was opened */
        fCloseClipboard = TRUE;

        /* Check that CF_UNICODETEXT clipboard format is available */
        /* (the system will synthesize from CF_TEXT if necessary, so we don't need to fall-back to that) */
        if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
          static int count;       /* Avoid spamming the log */
          static HWND lasthwnd;

          if (hwnd != lasthwnd)
            count = 0;
          count++;
          if (count < 6)
            ErrorF("winClipboardFlushXEvents - CF_UNICODETEXT is not available from Win32 clipboard.  Aborting %d.\n",
                   count);
          lasthwnd = hwnd;

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

        /* Setup the string style */
        const char *requestedClipboardEncoding = encodingNameFromAtom(e->target);

        /* Retrieve clipboard data */
        hGlobal = GetClipboardData(CF_UNICODETEXT);

        if (!hGlobal) {
          ErrorF("winClipboardFlushXEvents - SelectionRequest - GetClipboardData() failed: %08lx\n", GetLastError());

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

        pszGlobalData = (char *) GlobalLock(hGlobal);

        /* Find out how much space needed to convert UTF-16 WideChar string to UTF-8 MBCS */
        /* NOTE: iConvertDataLen includes space for null terminator */
        iConvertDataLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) pszGlobalData, -1, NULL, 0, NULL, NULL);

        /* Allocate memory for the UTF-8 string */
        pszConvertData = (char *) malloc(iConvertDataLen);

        /* Do the actual conversion */
        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) pszGlobalData, -1, pszConvertData, iConvertDataLen, NULL, NULL);

        /* Convert DOS string to UNIX string */
        winClipboardDOStoUNIX(pszConvertData, strlen(pszConvertData));

        /* Convert clipboard string to requested encoding */
        size_t outlength;
        char *outbuf = convertText(requestedClipboardEncoding, "UTF-8", pszConvertData, iConvertDataLen, &outlength);
        if (!outbuf)
          {
            ErrorF("winClipboardFlushXEvents - SelectionRequest - iconv conversion failed");

            /* Abort */
            fAbort = TRUE;
            goto winClipboardFlushXEvents_SelectionRequest_Done;
          }

        /* Free the converted string */
        free(pszConvertData);
        pszConvertData = NULL;

        /* Copy the clipboard text to the requesting window */
        xcb_void_cookie_t cookie = xcb_change_property_checked(conn,
                                                               XCB_PROP_MODE_REPLACE,
                                                               e->requestor,
                                                               e->property,
                                                               e->target,
                                                               8,
                                                               outlength,
                                                               outbuf);

        xcb_generic_error_t *error = xcb_request_check(conn, cookie);
        if (error) {
          ErrorF("winClipboardFlushXEvents - SelectionRequest - "
                 "xcb_change_property failed\n");

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

        /* Release the clipboard data */
        GlobalUnlock(hGlobal);
        pszGlobalData = NULL;
        fCloseClipboard = FALSE;
        CloseClipboard();

        /* Clean up */
        free(outbuf);
        outbuf = NULL;

        /* Setup selection notify event */
        xcb_selection_notify_event_t eventSelection;
        eventSelection.response_type = SelectionNotify;
        eventSelection.requestor = e->requestor;
        eventSelection.selection = e->selection;
        eventSelection.target = e->target;
        eventSelection.property = e->property;
        eventSelection.time = e->time;

        /* Notify the requesting window that the operation has completed */
        cookie = xcb_send_event_checked(conn, FALSE, eventSelection.requestor,
                                        0, (const char *)&eventSelection);

        error = xcb_request_check(conn, cookie);

        if (error) {
          ErrorF("winClipboardFlushXEvents - SelectionRequest - send event failed\n");

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_SelectionRequest_Done;
        }

      winClipboardFlushXEvents_SelectionRequest_Done:
        /* Free allocated resources */
        free(outbuf);
        outbuf = NULL;

        free(pszConvertData);
        if (hGlobal && pszGlobalData)
          GlobalUnlock(hGlobal);

        /*
         * Send a SelectionNotify event to the requesting
         * client when we abort.
         */
        if (fAbort) {
          winDebug("SelectionRequest - aborting\n");

          /* Setup selection notify event */
          xcb_selection_notify_event_t eventSelection;
          eventSelection.response_type = SelectionNotify;
          eventSelection.requestor = e->requestor;
          eventSelection.selection = e->selection;
          eventSelection.target = e->target;
          eventSelection.property = XCB_NONE; /* indicates failure */
          eventSelection.time = e->time;

          /* Notify the requesting window that the operation has failed */
          cookie = xcb_send_event_checked(conn, FALSE, eventSelection.requestor,
                                          0, (const char *)&eventSelection);

          error = xcb_request_check(conn, cookie);

          if (error) {
            /*
             * Should not be a problem if XSendEvent fails because
             * the client may simply have exited.
             */
            ErrorF("winClipboardFlushXEvents - SelectionRequest - send event failed for abort event\n");
          }
        }

        /* Close clipboard if it was opened */
        if (fCloseClipboard) {
          fCloseClipboard = FALSE;
          CloseClipboard();
        }
        break;
      }

      /*
       * SelectionNotify
       */

    case XCB_SELECTION_NOTIFY:
      {
        xcb_selection_notify_event_t *e = (xcb_selection_notify_event_t *)event;

        winDebug("winClipboardFlushXEvents - SelectionNotify\n");
        {
          xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, e->selection);
          xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);

          winDebug("SelectionNotify - Selection atom name %s\n", reply ? xcb_get_atom_name_name(reply) : "unknown");
          free(reply);
        }

        /*
         * Request conversion of UTF8 and CompoundText targets.
         */
        if (e->property == XCB_NONE) {
          if (e->target == XA_STRING) {
            winDebug("winClipboardFlushXEvents - SelectionNotify - XA_STRING\n");

            return WIN_XEVENTS_CONVERT;
          }
          else if (e->target == atomUTF8String) {
            winDebug("winClipboardFlushXEvents - SelectionNotify - Requesting conversion of UTF8 target.\n");

            xcb_convert_selection(conn, iWindow, e->selection, XA_STRING, atomLocalProperty, XCB_CURRENT_TIME);

            /* Process the ConvertSelection event */
            xcb_flush(conn);

            return WIN_XEVENTS_CONVERT;
          }
          else if (e->target == atomCompoundText) {
            winDebug("winClipboardFlushXEvents - SelectionNotify - Requesting conversion of CompoundText target.\n");

            xcb_convert_selection(conn, iWindow, e->selection, atomUTF8String, atomLocalProperty, XCB_CURRENT_TIME);

            /* Process the ConvertSelection event */
            xcb_flush(conn);

            return WIN_XEVENTS_CONVERT;
          }
          else {
            ErrorF("winClipboardFlushXEvents - SelectionNotify - Unknown format.  Cannot request conversion, aborting.\n");
            break;
          }
        }
      }

    case XCB_SELECTION_CLEAR:
      winDebug("SelectionClear - doing nothing\n");
      break;

    case XCB_PROPERTY_NOTIFY:
      {
        xcb_property_notify_event_t *e = (xcb_property_notify_event_t *)event;

        {
          xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, e->atom);
          xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);

          winDebug("PropertyNotify - ATOM: %s\n", reply ? xcb_get_atom_name_name(reply) : "unknown");
          free(reply);
        }

        if (e->atom != atomLocalProperty)
          break;

        /* Retrieve the size of the selection data */
        xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, iWindow, atomLocalProperty, XCB_GET_PROPERTY_TYPE_ANY,
                                                            0, 0);    /* Don't get data, just size */
        xcb_generic_error_t *error;
        xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, &error);

        if (error) {
          ErrorF("winClipboardFlushXEvents - PropertyNotify - XGetWindowProperty () failed, aborting: %d\n", error->error_code);
          free(error);
          break;
        }

        winDebug("PropertyNotify - length %d\n", reply->bytes_after);

        /* Retrieve the selection data */
        cookie = xcb_get_property(conn, 0, iWindow, atomLocalProperty, XCB_GET_PROPERTY_TYPE_ANY,
                                  0, reply->bytes_after);

        free(reply);

        reply = xcb_get_property_reply(conn, cookie, &error);
        if (error)  {
          ErrorF("winClipboardFlushXEvents - PropertyNotify - xcb_get_property() failed, aborting: %d\n", error->error_code);
          free(error);
        }

        winDebug("PropertyNotify - returned data %d left %d\n", reply->value_len, reply->bytes_after);

        {
          xcb_get_atom_name_cookie_t atom_cookie = xcb_get_atom_name(conn, reply->type);
          xcb_get_atom_name_reply_t *atom_reply = xcb_get_atom_name_reply(conn, atom_cookie, NULL);

          winDebug("PropertyNotify - encoding atom name %s\n", reply ? xcb_get_atom_name_name(atom_reply) : "unknown");
          free(reply);
        }

        const char *specifiedClipboardEncoding = encodingNameFromAtom(reply->type);

        /* Convert clipboard string from specified encoding */
        size_t outlength;
        char *pszReturnData = convertText("UTF-8", specifiedClipboardEncoding, xcb_get_property_value(reply), xcb_get_property_value_length(reply), &outlength);

        if (!pszReturnData) {
          ErrorF("winClipboardFlushXEvents - PropertyNotify - conversion failed\n");
          pszReturnData = malloc(1);
          pszReturnData[0] = '\0';
        }

        /* Free the data returned from xcb_get_property */
        free(reply);

        /* Convert the X clipboard string to DOS format */
        winClipboardUNIXtoDOS(&pszReturnData, strlen(pszReturnData));

        /* Find out how much space needed to convert UTF-8 MBCS to UTF-16 WideChar */
        iUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, pszReturnData, -1, NULL, 0);

        /* Allocate memory for the UTF-16 string */
        pwszUnicodeStr = (wchar_t *) malloc(sizeof(wchar_t) * (iUnicodeLen + 1));
        if (!pwszUnicodeStr) {
          ErrorF("winClipboardFlushXEvents - PropertyNotify - malloc failed for pwszUnicodeStr, aborting.\n");

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_PropertyNotify_Done;
        }

        /* Do the actual conversion */
        MultiByteToWideChar(CP_UTF8, 0, pszReturnData, -1, pwszUnicodeStr, iUnicodeLen);

        /* Allocate global memory for the Windows clipboard data */
        hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (iUnicodeLen + 1));

        free(pszReturnData);

        /* Check that global memory was allocated */
        if (!hGlobal) {
          ErrorF("winClipboardFlushXEvents - PropertyNotify - GlobalAlloc failed, aborting: %ld\n", GetLastError());
          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_PropertyNotify_Done;
        }

        /* Obtain a pointer to the global memory */
        pszGlobalData = GlobalLock(hGlobal);
        if (pszGlobalData == NULL) {
          ErrorF("winClipboardFlushXEvents - Could not lock global memory for clipboard transfer\n");

          /* Abort */
          fAbort = TRUE;
          goto winClipboardFlushXEvents_PropertyNotify_Done;
        }

        /* Copy the returned string into the global memory */
        memcpy(pszGlobalData, pwszUnicodeStr, sizeof(wchar_t) * (iUnicodeLen + 1));
        free(pwszUnicodeStr);
        pwszUnicodeStr = NULL;

        /* Release the pointer to the global memory */
        GlobalUnlock(hGlobal);
        pszGlobalData = NULL;

        /* Push the selection data to the Windows clipboard */
        SetClipboardData(CF_UNICODETEXT, hGlobal);

        /* Flag that SetClipboardData has been called */
        fSetClipboardData = FALSE;

        /*
         * NOTE: Do not try to free pszGlobalData, it is owned by
         * Windows after the call to SetClipboardData ().
         */

      winClipboardFlushXEvents_PropertyNotify_Done:
        free(pszConvertData);
        free(pwszUnicodeStr);
        if (hGlobal && pszGlobalData)
          GlobalUnlock(hGlobal);
        if (fSetClipboardData) {
          SetClipboardData(CF_UNICODETEXT, NULL);
          SetClipboardData(CF_TEXT, NULL);
        }
        return WIN_XEVENTS_NOTIFY;
      }

    case XCB_KEYMAP_NOTIFY:
      break;

    default:
      if ((event->response_type & ~0x80) == XCB_XFIXES_SELECTION_NOTIFY + xfixes_event_base)
        {
          xcb_xfixes_selection_notify_event_t *e = (xcb_xfixes_selection_notify_event_t *)event;

          if (e->subtype == XCB_XFIXES_SELECTION_EVENT_SET_SELECTION_OWNER)
            {
              /* Save selection owners for monitored selections, ignore other selections */
              if (e->selection == XA_PRIMARY)
                {
                  MonitorSelection(e, CLIP_OWN_PRIMARY);
                }
              else if (e->selection == atomClipboard)
                {
                  MonitorSelection(e, CLIP_OWN_CLIPBOARD);
                }
            }

          /* XCB_XFIXES_SELECTION_EVENT_SELECTION_WINDOW_DESTROY */
          /* XCB_XFIXES_SELECTION_EVENT_SELECTION_CLIENT_CLOSE */
        }
      else
        {
          ErrorF ("winClipboardFlushXEvents - unexpected event type %d\n", event->response_type);
        }
      break;
    }
  }

  return WIN_XEVENTS_SUCCESS;
}
