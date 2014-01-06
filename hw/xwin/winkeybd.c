/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "wmutil/scancodes.h"
#include "wmutil/keyboard.h"
#include "winconfig.h"
#include "winmsg.h"

#include "xkbsrv.h"
#include "dixgrabs.h"

/* C does not have a logical XOR operator, so we use a macro instead */
#define LOGICAL_XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))

#define AltMask		Mod1Mask
#define NumLockMask	Mod2Mask
#define AltLangMask	Mod3Mask
#define KanaMask	Mod4Mask
#define ScrollLockMask	Mod5Mask

/*
 * Local prototypes
 */

static void
 winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, pointer pCtrl, int iClass);

static void
 winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl);

/* Ring the keyboard bell (system speaker on PCs) */
static void
winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, pointer pCtrl, int iClass)
{
    /*
     * We can't use Beep () here because it uses the PC speaker
     * on NT/2000.  MessageBeep (MB_OK) will play the default system
     * sound on systems with a sound card or it will beep the PC speaker
     * on systems that do not have a sound card.
     */
    MessageBeep(MB_OK);
}

/* Change some keyboard configuration parameters */
static void
winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl)
{
}

/*
 * See Porting Layer Definition - p. 18
 * winKeybdProc is known as a DeviceProc.
 */

int
winKeybdProc(DeviceIntPtr pDeviceInt, int iState)
{
    DevicePtr pDevice = (DevicePtr) pDeviceInt;
    XkbSrvInfoPtr xkbi;
    XkbControlsPtr ctrl;

    switch (iState) {
    case DEVICE_INIT:
        winConfigKeyboard(pDeviceInt);

        /* FIXME: Maybe we should use winGetKbdLeds () here? */
        defaultKeyboardControl.leds = g_winInfo.keyboard.leds;

        winErrorFVerb(2, "Rules = \"%s\" Model = \"%s\" Layout = \"%s\""
                      " Variant = \"%s\" Options = \"%s\"\n",
                      g_winInfo.xkb.rules ? g_winInfo.xkb.rules : "none",
                      g_winInfo.xkb.model ? g_winInfo.xkb.model : "none",
                      g_winInfo.xkb.layout ? g_winInfo.xkb.layout : "none",
                      g_winInfo.xkb.variant ? g_winInfo.xkb.variant : "none",
                      g_winInfo.xkb.options ? g_winInfo.xkb.options : "none");

        InitKeyboardDeviceStruct(pDeviceInt,
                                 &g_winInfo.xkb, winKeybdBell, winKeybdCtrl);

        xkbi = pDeviceInt->key->xkbInfo;
        if ((xkbi != NULL) && (xkbi->desc != NULL)) {
            ctrl = xkbi->desc->ctrls;
            ctrl->repeat_delay = g_winInfo.keyboard.delay;
            ctrl->repeat_interval = 1000 / g_winInfo.keyboard.rate;
        }
        else {
            winErrorFVerb(1,
                          "winKeybdProc - Error initializing keyboard AutoRepeat\n");
        }

        break;

    case DEVICE_ON:
        pDevice->on = TRUE;

        // immediately copy the state of this keyboard device to the VCK
        // (which otherwise happens lazily after the first keypress)
        CopyKeyClass(pDeviceInt, inputInfo.keyboard);
        break;

    case DEVICE_CLOSE:
    case DEVICE_OFF:
        pDevice->on = FALSE;
        break;
    }

    return Success;
}

/*
 * Detect current mode key states upon server startup.
 *
 * Simulate a press and release of any key that is currently
 * toggled.
 */

void
winInitializeModeKeyStates(void)
{
    /* Restore NumLock */
    if (GetKeyState(VK_NUMLOCK) & 0x0001) {
        winSendKeyEvent(KEY_NumLock, TRUE);
        winSendKeyEvent(KEY_NumLock, FALSE);
    }

    /* Restore CapsLock */
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        winSendKeyEvent(KEY_CapsLock, TRUE);
        winSendKeyEvent(KEY_CapsLock, FALSE);
    }

    /* Restore ScrollLock */
    if (GetKeyState(VK_SCROLL) & 0x0001) {
        winSendKeyEvent(KEY_ScrollLock, TRUE);
        winSendKeyEvent(KEY_ScrollLock, FALSE);
    }

    /* Restore KanaLock */
    if (GetKeyState(VK_KANA) & 0x0001) {
        winSendKeyEvent(KEY_HKTG, TRUE);
        winSendKeyEvent(KEY_HKTG, FALSE);
    }
}

/*
 * Upon regaining the keyboard focus we must
 * resynchronize our internal mode key states
 * with the actual state of the keys.
 */

void
winRestoreModeKeyStates(void)
{
    DWORD dwKeyState;
    BOOL processEvents = TRUE;
    unsigned short internalKeyStates;

    /* X server is being initialized */
    if (!inputInfo.keyboard)
        return;

    /* Only process events if the rootwindow is mapped. The keyboard events
     * will cause segfaults otherwise */
    if (screenInfo.screens[0]->root &&
        screenInfo.screens[0]->root->mapped == FALSE)
        processEvents = FALSE;

    /* Force to process all pending events in the mi event queue */
    if (processEvents)
        mieqProcessInputEvents();

    /* Read the mode key states of our X server */
    /* (stored in the virtual core keyboard) */
    internalKeyStates =
        XkbStateFieldFromRec(&inputInfo.keyboard->key->xkbInfo->state);
    winDebug("winRestoreModeKeyStates: state %d\n", internalKeyStates);

    /* Check if modifier keys are pressed, and if so, fake a press */
    {

        BOOL lctrl = (GetAsyncKeyState(VK_LCONTROL) < 0);
        BOOL rctrl = (GetAsyncKeyState(VK_RCONTROL) < 0);
        BOOL lshift = (GetAsyncKeyState(VK_LSHIFT) < 0);
        BOOL rshift = (GetAsyncKeyState(VK_RSHIFT) < 0);
        BOOL alt = (GetAsyncKeyState(VK_LMENU) < 0);
        BOOL altgr = (GetAsyncKeyState(VK_RMENU) < 0);

        /*
           If AltGr and CtrlL appear to be pressed, assume the
           CtrL is a fake one
         */
        if (lctrl && altgr)
            lctrl = FALSE;

        if (lctrl)
            winSendKeyEvent(KEY_LCtrl, TRUE);

        if (rctrl)
            winSendKeyEvent(KEY_RCtrl, TRUE);

        if (lshift)
            winSendKeyEvent(KEY_ShiftL, TRUE);

        if (rshift)
            winSendKeyEvent(KEY_ShiftL, TRUE);

        if (alt)
            winSendKeyEvent(KEY_Alt, TRUE);

        if (altgr)
            winSendKeyEvent(KEY_AltLang, TRUE);
    }

    /*
       Check if latching modifier key states have changed, and if so,
       fake a press and a release to toggle the modifier to the correct
       state
    */
    dwKeyState = GetKeyState(VK_NUMLOCK) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & NumLockMask, dwKeyState)) {
        winSendKeyEvent(KEY_NumLock, TRUE);
        winSendKeyEvent(KEY_NumLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_CAPITAL) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & LockMask, dwKeyState)) {
        winSendKeyEvent(KEY_CapsLock, TRUE);
        winSendKeyEvent(KEY_CapsLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_SCROLL) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & ScrollLockMask, dwKeyState)) {
        winSendKeyEvent(KEY_ScrollLock, TRUE);
        winSendKeyEvent(KEY_ScrollLock, FALSE);
    }

    dwKeyState = GetKeyState(VK_KANA) & 0x0001;
    if (LOGICAL_XOR(internalKeyStates & KanaMask, dwKeyState)) {
        winSendKeyEvent(KEY_HKTG, TRUE);
        winSendKeyEvent(KEY_HKTG, FALSE);
    }

    /*
       For strict correctness, we should also press any non-modifier keys
       which are already down when we gain focus, but nobody has complained
       yet :-)
     */
}

void
winSendKeyEventCallback(DWORD dwKey, bool fDown)
{
#ifdef HAS_DEVWINDOWS
    /* Verify that the mi input system has been initialized */
    if (g_fdMessageQueue == WIN_FD_INVALID)
        return;
#endif

    QueueKeyboardEvents(g_pwinKeyboard, fDown ? KeyPress : KeyRelease,
                        dwKey, NULL);
}

/*
 */
int
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
{
    XkbAnyAction *xf86act = &(act->any);
    char msgbuf[XkbAnyActionDataSize+1];

    if (xf86act->type == XkbSA_XFree86Private)
      {
        memcpy(msgbuf, xf86act->data, XkbAnyActionDataSize);
        msgbuf[XkbAnyActionDataSize]= '\0';
        if (strcasecmp(msgbuf, "prgrbs")==0) {
            DeviceIntPtr tmp;
            winMsg(X_INFO, "Printing all currently active device grabs:\n");
            for (tmp = inputInfo.devices; tmp; tmp = tmp->next)
                if (tmp->deviceGrab.grab)
                    PrintDeviceGrabInfo(tmp);
            winMsg(X_INFO, "End list of active device grabs\n");
        }
        else if (strcasecmp(msgbuf, "ungrab")==0)
            UngrabAllDevices(FALSE);
        else if (strcasecmp(msgbuf, "clsgrb")==0)
            UngrabAllDevices(TRUE);
        else if (strcasecmp(msgbuf, "prwins")==0)
            PrintWindowTree();
    }

    return 0;
}
