/*
  File: winclipboard_globals.h
  Purpose: declarations for global variables for clipboard

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice (including the next
  paragraph) shall be included in all copies or substantial portions of the
  Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

*/

#ifndef WINCLIPBOARD_GLOBALS_H
#define WINCLIPBOARD_GLOBALS_H

/*
 * Clipboard variables
 */

#ifdef XWIN_CLIPBOARD
extern Bool			g_fUnicodeClipboard;
extern Bool			g_fClipboardLaunched;
extern Bool			g_fClipboardStarted;
extern pthread_t		g_ptClipboardProc;
extern HWND			g_hwndClipboard;
extern Bool			g_fClipboard;
extern void		*g_pClipboardDisplay;
extern Window		g_iClipboardWindow;
extern Bool		g_fUnicodeSupport;
extern Atom		g_atomLastOwnedSelection;
#endif

#endif /* WINCLIPBOARD_GLOBALS_H */
