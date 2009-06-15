#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xwindows.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <glx/glxserver.h>
#include <glx/glxext.h>

#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
//#include <glx/glxserver.h>
#include <glx/glxscreens.h>
#include <glx/glxdrawable.h>
#include <glx/glxcontext.h>
//#include <glx/glxext.h>
#include <glx/glxutil.h>
//#include <glx/glxscreens.h>
#include <GL/internal/glcore.h>
#include <stdlib.h>


typedef struct {
    unsigned enableDebug : 1;
    unsigned enableTrace : 1;
    unsigned dumpPFD : 1;
    unsigned dumpHWND : 1;
    unsigned dumpDC : 1;
    unsigned enableStereo : 1;
} glWinDebugSettingsRec, *glWinDebugSettingsPtr;
extern glWinDebugSettingsRec glWinDebugSettings;

void setup_dispatch_table(void);

#if 1
#define GLWIN_TRACE() if (glWinDebugSettings.enableTrace) ErrorF("%s:%d: Trace\n", __FUNCTION__, __LINE__ )
#define GLWIN_TRACE_MSG(msg, args...) if (glWinDebugSettings.enableTrace) ErrorF("%s:%d: " msg, __FUNCTION__, __LINE__, ##args )
#define GLWIN_DEBUG_MSG(msg, args...) if (glWinDebugSettings.enableDebug) ErrorF("%s:%d: " msg, __FUNCTION__, __LINE__, ##args )
#define GLWIN_DEBUG_MSG2(msg, args...) if (glWinDebugSettings.enableDebug) ErrorF(msg, ##args )
#else
#define GLWIN_TRACE()
#define GLWIN_TRACE_MSG(a, ...)
#define GLWIN_DEBUG_MSG(a, ...)
#define GLWIN_DEBUG_MSG2(a, ...)
#endif

