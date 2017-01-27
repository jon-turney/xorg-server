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

#define WINVER 0x0600

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "wmutil/scancodes.h"
#include "wmutil/keyboard.h"
#include "winconfig.h"
#include "winmsg.h"
#include "winlayouts.h"

#include "xkbsrv.h"
#include "dixgrabs.h"
#include "keysym2ucs.h"

/* C does not have a logical XOR operator, so we use a macro instead */
#define LOGICAL_XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))

#define AltMask		Mod1Mask
#define NumLockMask	Mod2Mask
#define AltLangMask	Mod3Mask
#define KanaMask	Mod4Mask
#define ScrollLockMask	Mod5Mask

extern void *ExecAndLogThread(void *cmd);

/*
 * Local prototypes
 */

static void
 winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, void *pCtrl, int iClass);

static void
 winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl);

/* */


/* Ring the keyboard bell (system speaker on PCs) */
static void
winKeybdBell(int iPercent, DeviceIntPtr pDeviceInt, void *pCtrl, int iClass)
{
    /*
     * We can't use Beep () here because it uses the PC speaker
     * on NT/2000.  MessageBeep (MB_OK) will play the default system
     * sound on systems with a sound card or it will beep the PC speaker
     * on systems that do not have a sound card.
     */
    if (iPercent > 0) MessageBeep(MB_OK);
}

/* Change some keyboard configuration parameters */
static void
winKeybdCtrl(DeviceIntPtr pDevice, KeybdCtrl * pCtrl)
{
}

/*

 */

/*
  Each key can generate 4 glyphs. They are, in order:
  unshifted, shifted, modeswitch unshifted, modeswitch shifted
*/
#define GLYPHS_PER_KEY 4
#define NUM_KEYCODES   248
#define MIN_KEYCODE    XkbMinLegalKeyCode      /* unfortunately, this isn't 0... */
#define MAX_KEYCODE    NUM_KEYCODES + MIN_KEYCODE - 1

typedef struct winKeyboardInfo_struct {
    CARD8 modMap[MAP_LENGTH];
    KeySym keyMap[MAP_LENGTH * GLYPHS_PER_KEY];
} winKeyboardInfo;

winKeyboardInfo keyInfo;

/*
 * BuildModifierMaps
 *
 *      Populate modMap assigning modifier-type keys which are present in the
 *      keymap to specific modifiers.
 *
 */
static void
BuildModifierMaps(winKeyboardInfo *info)
{
    int i;
    KeySym *k;

    memset(info->modMap, NoSymbol, sizeof(info->modMap));

    for (i = 0; i < NUM_KEYCODES; i++) {
        k = info->keyMap + i * GLYPHS_PER_KEY;

        switch (*k) {
        case XK_Shift_L:
            info->modMap[MIN_KEYCODE + i] = ShiftMask;
            break;

        case XK_Shift_R:
            info->modMap[MIN_KEYCODE + i] = ShiftMask;
            break;

        case XK_Control_L:
            info->modMap[MIN_KEYCODE + i] = ControlMask;
            break;

        case XK_Control_R:
            info->modMap[MIN_KEYCODE + i] = ControlMask;
            break;

        case XK_Caps_Lock:
            info->modMap[MIN_KEYCODE + i] = LockMask;
            break;

        case XK_Alt_L:
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Alt_R:
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Meta_L:
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Meta_R:
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Num_Lock:
            info->modMap[MIN_KEYCODE + i] = Mod2Mask;
            break;

        case XK_Mode_switch:
            info->modMap[MIN_KEYCODE + i] = Mod5Mask;
            break;

        case XK_ISO_Level3_Shift:
            info->modMap[MIN_KEYCODE + i] = Mod5Mask;
            break;
        }
    }
}

/* Table of virtualkey->keysym mappings for keys that are not Unicode characters */
const static struct {
    int vk;
    KeySym ks[GLYPHS_PER_KEY];
} knownVKs[] = {
    { VK_BACK    , {XK_BackSpace, XK_BackSpace, XK_BackSpace, XK_BackSpace} },
    { VK_ESCAPE  , {XK_Escape, XK_Escape, XK_Escape, XK_Escape} },
    { VK_RETURN  , {XK_Return, XK_Return, XK_Return, XK_Return} },
    { VK_TAB     , {XK_Tab, XK_Tab, XK_Tab, XK_Tab} },

    /* Modifier keys */
    { VK_CONTROL , { XK_Control_L, XK_Control_L, XK_Control_L, XK_Control_L} },
    { VK_LCONTROL, { XK_Control_L, XK_Control_L, XK_Control_L, XK_Control_L} },
    { VK_RCONTROL, { XK_Control_R, XK_Control_R, XK_Control_R, XK_Control_R} },
    { VK_SHIFT   , { XK_Shift_L, XK_Shift_L, XK_Shift_L, XK_Shift_L} },
    { VK_LSHIFT  , { XK_Shift_L, XK_Shift_L, XK_Shift_L, XK_Shift_L} },
    { VK_RSHIFT  , { XK_Shift_R, XK_Shift_R, XK_Shift_R, XK_Shift_R} },
    { VK_MENU    , { XK_Alt_L, XK_Alt_L, XK_Alt_L, XK_Alt_L} },
    { VK_LMENU   , { XK_Alt_L, XK_Alt_L, XK_Alt_L, XK_Alt_L} },
    /*  VK_RMENU ties up with the keyboard state used for modeswitched states */
    { VK_RMENU   , { XK_ISO_Level3_Shift, XK_ISO_Level3_Shift, XK_ISO_Level3_Shift, XK_ISO_Level3_Shift} },
    { VK_CAPITAL , { XK_Caps_Lock, XK_Caps_Lock, XK_Caps_Lock, XK_Caps_Lock} },
    { VK_LWIN    , { XK_Super_L, XK_Super_L, XK_Super_L, XK_Super_L} },
    { VK_RWIN    , { XK_Super_R, XK_Super_R, XK_Super_R, XK_Super_R} },

    /* */
    { VK_CANCEL  , { XK_Break, XK_Break, XK_Break, XK_Break} },
    { VK_PAUSE   , { XK_Pause, XK_Pause, XK_Pause, XK_Pause} },
    { VK_SCROLL  , { XK_Scroll_Lock, XK_Scroll_Lock, XK_Scroll_Lock, XK_Scroll_Lock} },
    { VK_SNAPSHOT, { XK_Sys_Req, XK_Sys_Req, XK_Sys_Req, XK_Sys_Req} },
    { VK_APPS    , { XK_Menu, XK_Menu, XK_Menu, XK_Menu } },

    /* function keys */
    { VK_F1      , { XK_F1, XK_F1, XK_F1, XK_F1 } },
    { VK_F2      , { XK_F2, XK_F2, XK_F2, XK_F2 } },
    { VK_F3      , { XK_F3, XK_F3, XK_F3, XK_F3 } },
    { VK_F4      , { XK_F4, XK_F4, XK_F4, XK_F4 } },
    { VK_F5      , { XK_F5, XK_F5, XK_F5, XK_F5 } },
    { VK_F6      , { XK_F6, XK_F6, XK_F6, XK_F6 } },
    { VK_F7      , { XK_F7, XK_F7, XK_F7, XK_F7 } },
    { VK_F8      , { XK_F8, XK_F8, XK_F8, XK_F8 } },
    { VK_F9      , { XK_F9, XK_F9, XK_F9, XK_F9 } },
    { VK_F10     , { XK_F10, XK_F10, XK_F10, XK_F10 } },
    { VK_F11     , { XK_F11, XK_F11, XK_F11, XK_F11 } },
    { VK_F12     , { XK_F12, XK_F12, XK_F12, XK_F12 } },

    /* numpad */
    { VK_NUMLOCK , { XK_Num_Lock, XK_Num_Lock, XK_Num_Lock, XK_Num_Lock } },
    { VK_ADD     , { XK_KP_Add, XK_KP_Add, XK_KP_Add, XK_KP_Add } },
    { VK_DIVIDE  , { XK_KP_Divide, XK_KP_Divide, XK_KP_Divide, XK_KP_Divide } },
    { VK_MULTIPLY, { XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply } },
    { VK_SUBTRACT, { XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract } },
    { VK_DECIMAL , { XK_KP_Delete, XK_KP_Decimal, XK_KP_Delete, XK_KP_Decimal } },
    { VK_NUMPAD0 , { XK_KP_Insert, XK_KP_0, XK_KP_Insert, XK_KP_0 } },
    { VK_NUMPAD1 , { XK_KP_End, XK_KP_1, XK_KP_End, XK_KP_1 } },
    { VK_NUMPAD2 , { XK_KP_Down, XK_KP_2, XK_KP_Down, XK_KP_2 } },
    { VK_NUMPAD3 , { XK_KP_Next, XK_KP_3, XK_KP_Next, XK_KP_3 } },
    { VK_NUMPAD4 , { XK_KP_Left, XK_KP_4, XK_KP_Left, XK_KP_4 } },
    { VK_NUMPAD5 , { XK_KP_Begin, XK_KP_5, XK_KP_Begin, XK_KP_5 } },
    { VK_NUMPAD6 , { XK_KP_Right, XK_KP_6, XK_KP_Right, XK_KP_6 } },
    { VK_NUMPAD7 , { XK_KP_Home, XK_KP_7, XK_KP_Home, XK_KP_7 } },
    { VK_NUMPAD8 , { XK_KP_Up, XK_KP_8, XK_KP_Up, XK_KP_8 } },
    { VK_NUMPAD9 , { XK_KP_Prior, XK_KP_9, XK_KP_Prior, XK_KP_9 } },

    /* cursor motion */
    { VK_DELETE  , { XK_Delete, XK_Delete, XK_Delete, XK_Delete } },
    { VK_DOWN    , { XK_Down, XK_Down, XK_Down, XK_Down } },
    { VK_END     , { XK_End, XK_End, XK_End, XK_End } },
    { VK_HOME    , { XK_Home, XK_Home, XK_Home, XK_Home } },
    { VK_INSERT  , { XK_Insert, XK_Insert, XK_Insert, XK_Insert } },
    { VK_LEFT    , { XK_Left, XK_Left, XK_Left, XK_Left } },
    { VK_NEXT    , { XK_Page_Down, XK_Page_Down, XK_Page_Down, XK_Page_Down } },
    { VK_PRIOR   , { XK_Page_Up, XK_Page_Up, XK_Page_Up, XK_Page_Up } },
    { VK_RIGHT   , { XK_Right, XK_Right, XK_Right, XK_Right } },
    { VK_UP      , { XK_Up, XK_Up, XK_Up, XK_Up } },
};

static KeySym
VKToKeySym(UINT vk, int state)
{
    int i;

    for (i = 0; i < sizeof(knownVKs) / sizeof(knownVKs[0]); i++)
        if (knownVKs[i].vk == vk) return knownVKs[i].ks[state];

    return 0;
}

const static struct {
    WCHAR diacritic;
    KeySym dead;
} dead_keys[] = {
    /* This table is sorted by KeySym value */
    { L'`',  XK_dead_grave },       /* U+0060 GRAVE ACCENT */
    { L'\'', XK_dead_acute },       /* U+0027 APOSTROPHE */
    { L'´',  XK_dead_acute },       /* U+00B4 ACUTE ACCENT */
    { L'^',  XK_dead_circumflex },  /* U+005E CIRCUMFLEX ACCENT */
    { L'~',  XK_dead_tilde },       /* U+007E TILDE */
    { L'¯',  XK_dead_macron },      /* U+00AF MARK */
    { L'˘',  XK_dead_breve },       /* U+02D8 BREVE */
    { L'˙',  XK_dead_abovedot },    /* U+02D9 DOT ABOVE */
    { L'¨',  XK_dead_diaeresis },   /* U+00A8 DIAERESIS */
    { L'"',  XK_dead_diaeresis },   /* U+0022 QUOTATION MARK */
    { L'°',  XK_dead_abovering },   /* U+00B0 DEGREE SIGN */
    { L'˝',  XK_dead_doubleacute }, /* U+02DD DOUBLE ACUTE ACCENT */
    { L'ˇ',  XK_dead_caron },       /* U+02C7 CARON */
    { L'¸',  XK_dead_cedilla },     /* U+00B8 CEDILLA */
    { L'˛',  XK_dead_ogonek },      /* U+02DB OGONEK */
    /* This table is incomplete ... */
};

/* Map the spacing form of a diacritic glyph to a deadkey KeySym */
static KeySym
make_dead_key(WCHAR in)
{
    int i;

    for (i = 0; i < sizeof(dead_keys) / sizeof(dead_keys[0]); i++)
        if (dead_keys[i].diacritic == in) return dead_keys[i].dead;

    return 0;
}

static bool
winKeybdModmapLayout(void)
{
    UINT vk;
    int j;
    memset(keyInfo.modMap, 0, MAP_LENGTH);
    memset(keyInfo.keyMap, 0, MAP_LENGTH * GLYPHS_PER_KEY * sizeof(KeySym));

    /*
      For all scancodes, in the four shift states

      If it produces a VirtualKey code for which we know the
      corresponding KeySym, use that.

      If it produces a Unicode character, convert that to an X11 KeySym
      (which may just be a unicode Keysym, if there isn't a better one)
    */
    for (j = 0; j < 4; j++) {
        /* Setup the modifier key state */
        BYTE state[256];
        memset(state, 0, 256);
        if ((j % 2) == 1)
            state[VK_SHIFT] = 0x80;
        if (j >= 2) {
            /* VK_RMENU is mapped to XK_ISO_Level3_Shift
               AltGR = Alt + Control
               (what to use here might depend on layout?) */
            state[VK_CONTROL] = 0x80;
            state[VK_MENU] = 0x80;
        }

        winDebug("Examining %s%s layout\n", (j % 2) ? "shift " : "",
                 (j >= 2) ? "altgr" : "");

        for (vk = 0; vk < 0xff; vk++) {
            int sc;
            KeySym ks;
            char keyName[64];

            /* Translate Virtual Key code to extended scan code */
            sc = MapVirtualKey(vk, MAPVK_VK_TO_VSC_EX);
            if (sc == 0)
                continue;

            /* Workaround what appear to be bugs in MAPVK_VK_TO_VSC_EX */
            if (((vk >= VK_PRIOR) && (vk <= VK_DOWN)) ||
                (vk == VK_INSERT) || (vk == VK_DELETE)) {
                /* cursor keys are extended keys */
                sc = sc | 0xe000;
            }

            /* Munge as we do actual key events, to translate extended
               scancodes into the server generated ones */
            {
                int tsc;
                tsc = winTranslateKey(vk,
                                      ((sc & 0xff) | (sc & 0xe000 ? 0x100 : 0)) << 16);

                if (tsc != sc) {
                    winDebug("scan code munged %x -> %x\n", sc, tsc);
                    sc = tsc;
                }
            }

            GetKeyNameText(sc << 16, keyName, sizeof(keyName));

            /* Is the KeySym for the VK_ known ?*/
            ks = VKToKeySym(vk, j);
            if (ks) {
                winDebug("scan code %x (%s), VK %x -> keysym %x\n", sc, keyName, vk, ks);
            } else {
                /* What unicode characters are output by that key in
                   this modifier state */
                int result;
                WCHAR out[8];
                char outAsUtf8[8];
                memset(outAsUtf8, 0, 8);

                result = ToUnicode(vk, sc, state, &out[0], 8, 0);

                if (result != 0) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, out, 1, outAsUtf8, 8, NULL, NULL);
                    outAsUtf8[len] = '\0';
                }

                switch (result) {
                case -1: /* dead-key */
                    ks = make_dead_key(out[0]);
                    if (ks)
                        winDebug("scan code %x (%s), VK %x -> deadkey %x ('%s') -> keysym %x\n", sc, keyName, vk, out[0], outAsUtf8, ks);
                    else
                        ErrorF("scan code %x (%s), VK %x -> deadkey %x ('%s') -> unknown keysym\n", sc, keyName, vk, out[0], outAsUtf8);

                    /* Force the partially-composed state to be discarded, so it
                       doesn't effect the next call to ToUnicode() */
                    ToUnicode(vk, sc, state, &out[0], 8, 0);

                    break;
                case 0: /* nothing */
                    winDebug("scan code %x (%s), VK %x -> nothing\n", sc, keyName, vk);
                    break;
                case 1: /* something */
                    ks = ucs2keysym(out[0]);
                    winDebug("scan code %x (%s), VK %x -> unicode %x ('%s') -> keysym %x\n", sc, keyName, vk, out[0], outAsUtf8, ks);
                    break;
                default:
                case 2: /* too much */
                    ErrorF("scan code %x (%s), VK %x produced %d unicode characters ('%s')\n", sc, keyName, vk, result, outAsUtf8);
                    break;
                }
            }

            if (ks) {
                KeySym *k;
                k = keyInfo.keyMap + (sc & 0xff) * GLYPHS_PER_KEY;
                if ((k[j]) && (k[j] != ks))
                    ErrorF("X KeyCode %d state %d KeySym collision %d and %d\n", sc, j, k[j], ks);
                k[j] = ks;
            }
        }

        /* keypad_enter doesn't have a separate VK code, so we need to
           add it ourselves */
        {
            int sc;
            KeySym *k;

            sc = KEY_KP_Enter;
            k = keyInfo.keyMap + (sc & 0xff) * GLYPHS_PER_KEY;
            k[j] = XK_KP_Enter;
        }
    }

    /* Build the modmap */
    BuildModifierMaps(&keyInfo);

    return TRUE;
}

/* Find the XKB layout corresponding to the current Windows keyboard layout */
void
winKeybdLayoutFind(void)
{
    char layoutName[KL_NAMELENGTH];
    unsigned char layoutFriendlyName[256];
    unsigned int layoutNum = 0;
    unsigned int deviceIdentifier = 0;
    int keyboardType;
    Bool bfound = FALSE;

    /* Setup defaults */
    XkbGetRulesDflts(&g_winInfo.xkb);

    g_winInfo.rmlvoNotModmap = TRUE;

    keyboardType = GetKeyboardType(0);
    if (keyboardType > 0 && GetKeyboardLayoutName(layoutName)) {
        WinKBLayoutPtr pLayout;
        int pass;

        layoutNum = strtoul(layoutName, (char **) NULL, 16);
        if ((layoutNum & 0xffff) == 0x411) {
            if (keyboardType == 7) {
                /* Japanese layouts have problems with key event messages
                   such as the lack of WM_KEYUP for Caps Lock key.
                   Loading US layout fixes this problem. */
                if (LoadKeyboardLayout("00000409", KLF_ACTIVATE) != NULL)
                    winMsg(X_INFO, "Loading US keyboard layout.\n");
                else
                    winMsg(X_ERROR, "LoadKeyboardLayout failed.\n");
            }
        }

        /* Discover the friendly name of the current layout */
        {
            HKEY regkey = NULL;
            const char regtempl[] =
                "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\";
            char *regpath;
            DWORD namesize = sizeof(layoutFriendlyName);

            regpath = malloc(sizeof(regtempl) + KL_NAMELENGTH + 1);
            strcpy(regpath, regtempl);
            strcat(regpath, layoutName);

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, regpath, &regkey))
                RegQueryValueEx(regkey, "Layout Text", 0, NULL,
                                layoutFriendlyName, &namesize);

            /* Close registry key */
            if (regkey)
                RegCloseKey(regkey);
            free(regpath);
        }

        winMsg(X_PROBED,
               "Windows keyboard layout: \"%s\" (%08x) \"%s\", type %d\n",
               layoutName, layoutNum, layoutFriendlyName, keyboardType);

        deviceIdentifier = layoutNum >> 16;
        for (pass = 0; pass < 2; pass++) {
            /* If we didn't find an exact match for the input locale identifer,
               try to find an match on the language identifier part only  */
            if (pass == 1)
                layoutNum = (layoutNum & 0xffff);

            for (pLayout = winKBLayouts; pLayout->winlayout != -1; pLayout++) {
                if (pLayout->winlayout != layoutNum)
                    continue;
                if (pLayout->winkbtype > 0 && pLayout->winkbtype != keyboardType)
                    continue;

                bfound = TRUE;
                winMsg(X_PROBED,
                       "Found matching XKB configuration \"%s\"\n",
                       pLayout->layoutname);

                g_winInfo.xkb.model = pLayout->xkbmodel;
                g_winInfo.xkb.layout = pLayout->xkblayout;
                g_winInfo.xkb.variant = pLayout->xkbvariant;
                g_winInfo.xkb.options = pLayout->xkboptions;

                if (deviceIdentifier == 0xa000) {
                    winMsg(X_PROBED, "Windows keyboard layout device identifier indicates Macintosh, setting Model = \"macintosh\"");
                    g_winInfo.xkb.model = "macintosh";
                }

                winMsg(X_PROBED,
                       "Rules = \"%s\" Model = \"%s\" Layout = \"%s\""
                       " Variant = \"%s\" Options = \"%s\"\n",
                      g_winInfo.xkb.rules ? g_winInfo.xkb.rules : "none",
                      g_winInfo.xkb.model ? g_winInfo.xkb.model : "none",
                      g_winInfo.xkb.layout ? g_winInfo.xkb.layout : "none",
                      g_winInfo.xkb.variant ? g_winInfo.xkb.variant : "none",
                      g_winInfo.xkb.options ? g_winInfo.xkb.options : "none");

                break;
            }

            if (bfound)
                break;
        }
    }

    /* If that fails, try converting the Windows layout to a modmap */
    if (!bfound) {
        winMsg(X_ERROR,
               "Keyboardlayout \"%s\" (%s) is unknown\n", layoutFriendlyName, layoutName);

        bfound = winKeybdModmapLayout();

        if (bfound) {
            winMsg(X_INFO, "Converted to modmap\n");
            g_winInfo.rmlvoNotModmap = FALSE;
        } else {
            winMsg(X_INFO, "Conversion to modmap failed\n");
        }
    }

    if (!bfound) {
        winMsg(X_ERROR, "using X server default layout\n");
    }
}

void
winKeybdLayoutChange(void)
{
    /* Ignore layout changes if static layout was requested */
    if (g_cmdline.xkbStatic)
        return;

    winKeybdLayoutFind();

    if (g_winInfo.rmlvoNotModmap) {
        pthread_t t;
        static char cmd[256];

        /* Compile rmlvo keymap and apply it */
        sprintf(cmd, "/usr/bin/setxkbmap %s%s %s%s %s%s %s%s %s%s",
                g_winInfo.xkb.rules ? "-rules " : "",
                g_winInfo.xkb.rules ? g_winInfo.xkb.rules : "",
                g_winInfo.xkb.model ? "-model " : "",
                g_winInfo.xkb.model ? g_winInfo.xkb.model : "",
                g_winInfo.xkb.layout ? "-layout " : "",
                g_winInfo.xkb.layout ? g_winInfo.xkb.layout : "",
                g_winInfo.xkb.variant ? "-variant " : "",
                g_winInfo.xkb.variant ? g_winInfo.xkb.variant : "",
                g_winInfo.xkb.options ? "-option " : "",
                g_winInfo.xkb.options ? g_winInfo.xkb.options : "");

        /*
          Really keymap should be updated synchronously in WM_INPUTLANGCHANGE,
          so that any subsequent WM_KEY(UP|DOWN) are interpreted correctly.  But
          this is currently impossible as message pump is run in the server
          thread, so we create a separate thread to do this work...
        */

        if (!pthread_create(&t, NULL, ExecAndLogThread, cmd))
            pthread_detach(t);
        else
            ErrorF("Creating setxkbmap failed\n");
    } else {
        KeySymsRec keySyms;
        keySyms.map = keyInfo.keyMap;
        keySyms.mapWidth = GLYPHS_PER_KEY;
        keySyms.minKeyCode = MIN_KEYCODE;
        keySyms.maxKeyCode = MAX_KEYCODE;

        /* TODO: We should build the entire XkbDescRec and use XkbCopyKeymap */
        XkbApplyMappingChange(g_pwinKeyboard, &keySyms, keySyms.minKeyCode,
                              keySyms.maxKeyCode - keySyms.minKeyCode + 1,
                              keyInfo.modMap, serverClient);

    }
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

        /*
         * Query the windows autorepeat settings and change the xserver defaults.
         */
        {
            int kbd_delay;
            DWORD kbd_speed;

            if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &kbd_delay, 0) &&
                SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &kbd_speed, 0)) {
                switch (kbd_delay) {
                case 0:
                    g_winInfo.keyboard.delay = 250;
                    break;
                case 1:
                    g_winInfo.keyboard.delay = 500;
                    break;
                case 2:
                    g_winInfo.keyboard.delay = 750;
                    break;
                default:
                case 3:
                    g_winInfo.keyboard.delay = 1000;
                    break;
                }
                g_winInfo.keyboard.rate = (kbd_speed > 0) ? kbd_speed : 1;
                winMsg(X_PROBED, "Setting autorepeat to delay=%ld, rate=%ld\n",
                       g_winInfo.keyboard.delay, g_winInfo.keyboard.rate);
            }
        }

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
    if (!inputInfo.keyboard || !inputInfo.keyboard->key)
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
                        dwKey);
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
