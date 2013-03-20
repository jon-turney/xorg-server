/*
 * File: keyboard.h
 * Purpose: interface for X->Windows keyboard event conversion
 *
 * Copyright (c) Jon TURNEY 2012
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

#ifndef WMUTILS_KEYBOARD_H
#define WMUTILS_KEYBOARD_H

#include <stdbool.h>

void winSendKeyEvent(DWORD dwKey, bool fDown);
void winSendKeyEventCallback(DWORD dwKey, bool fDown);
void winKeybdReleaseKeys(void);
bool winIsFakeCtrl_L(UINT message, WPARAM wParam, LPARAM lParam);
bool winCheckKeyPressed(WPARAM wParam, LPARAM lParam);
int winTranslateKey(WPARAM wParam, LPARAM lParam);
void winFixShiftKeys(int iScanCode);

#endif /* WMUTILS_KEYBOARD_H */
