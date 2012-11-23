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

#include <X11/Xwindows.h>
#include <X11/X.h>

#include "mouse.h"

/* Handle the mouse wheel */
void
winMouseWheel(int *iTotalDeltaZ, int iDeltaZ, int iButtonUp, int iButtonDown)
{
    int button;

    /* Do we have any previous delta stored? */
    if ((*iTotalDeltaZ > 0 && iDeltaZ > 0)
        || (*iTotalDeltaZ < 0 && iDeltaZ < 0)) {
        /* Previous delta and of same sign as current delta */
        iDeltaZ += *iTotalDeltaZ;
        *iTotalDeltaZ = 0;
    }
    else {
        /*
         * Previous delta of different sign, or zero.
         * We will set it to zero for either case,
         * as blindly setting takes just as much time
         * as checking, then setting if necessary :)
         */
        *iTotalDeltaZ = 0;
    }

    /*
     * Only process this message if the wheel has moved further than
     * WHEEL_DELTA
     */
    if (iDeltaZ >= WHEEL_DELTA || (-1 * iDeltaZ) >= WHEEL_DELTA) {
        *iTotalDeltaZ = 0;

        /* Figure out how many whole deltas of the wheel we have */
        iDeltaZ /= WHEEL_DELTA;
    }
    else {
        /*
         * Wheel has not moved past WHEEL_DELTA threshold;
         * we will store the wheel delta until the threshold
         * has been reached.
         */
        *iTotalDeltaZ = iDeltaZ;
        return;
    }

    /* Set the button to indicate up or down wheel delta */
    if (iDeltaZ > 0) {
        button = iButtonUp;
    }
    else {
        button = iButtonDown;
    }

    /*
     * Flip iDeltaZ to positive, if negative,
     * because always need to generate a *positive* number of
     * button clicks for the Z axis.
     */
    if (iDeltaZ < 0) {
        iDeltaZ *= -1;
    }

    /* Generate X input messages for each wheel delta we have seen */
    while (iDeltaZ--) {
        /* Push the wheel button */
        winMouseButtonsSendEvent(TRUE, button);

        /* Release the wheel button */
        winMouseButtonsSendEvent(FALSE, button);
    }
}
