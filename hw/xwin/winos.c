/*
 * Copyright (c) 2010-2014 Colin Harrison All Rights Reserved.
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
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 *
 * Author: Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

static const char*
IsWow64(void)
{
#ifdef __x86_64__
    return " (Win64)";
#else
    /* Check if we are running under WoW64 */
    LPFN_ISWOW64PROCESS fnIsWow64Process =
        (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                                             "IsWow64Process");
    if (NULL != fnIsWow64Process) {
        WINBOOL bIsWow64 = FALSE;

        if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
            return bIsWow64 ? " (WoW64)" : " (Win32)";
        }
        else {
            /* IsWow64Process() failed */
            return " (Unknown)";
        }
    }
    else {
        /* OS doesn't support IsWow64Process() */
        return "";
    }
#endif
}

/*
 * Report the OS version
 */

void
winOS(void)
{
    OSVERSIONINFOEX osvi = {0};
    const char *windowstype = "Unknown";
    const char *prodName = "Unknown";
    const char *isWow = "Unknown";

    /* Get operating system version information */
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((LPOSVERSIONINFO)&osvi);

    /* Branch on platform ID */
    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
        windowstype = "Windows NT";

        if (osvi.dwMajorVersion <= 4)
            prodName = "Windows NT";
        else if (osvi.dwMajorVersion == 10) {
            if (osvi.dwMinorVersion == 0) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 10";
                else
                    prodName = "Windows Server 2016";
            }
        }
        else if (osvi.dwMajorVersion == 6) {
            if (osvi.dwMinorVersion == 4) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 10";
                else
                    prodName = "Windows Server 2016";
            }
            if (osvi.dwMinorVersion == 3) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 8.1";
                else
                    prodName = "Windows Server 2012 R2";
            }
            if (osvi.dwMinorVersion == 2) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 8";
                else
                    prodName = "Windows Server 2012";
            }
            else if (osvi.dwMinorVersion == 1) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 7";
                else
                    prodName = "Windows Server 2008 R2";
            }
            else if (osvi.dwMinorVersion == 0) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows Vista";
                else
                    prodName = "Windows Server 2008";
            }
        }
        else if (osvi.dwMajorVersion == 5) {
            if (osvi.dwMinorVersion == 2) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows XP x64 Edition";
                else if (GetSystemMetrics(SM_SERVERR2))
                    prodName = "Windows Server 2003 R2";
                else
                    prodName = "Windows Server 2003";
            }
            else if (osvi.dwMinorVersion == 1)
                prodName = "Windows XP";
            else if (osvi.dwMinorVersion == 0) {
                prodName = "Windows 2000";
                break;
            }
        }

        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        windowstype = "Windows";
        break;
    }

    isWow = IsWow64();

    ErrorF("OS: %s %s [%s %d.%d build %d]%s\n",
           prodName, osvi.szCSDVersion,
           windowstype, (int)osvi.dwMajorVersion, (int)osvi.dwMinorVersion,
           (int)osvi.dwBuildNumber, isWow);
}
