/* swassertfail.cpp -- SWAssertFail (win 0x004794d0, donor SpriteWorldUtils.c line 1866). Notes: Research/notes/swassertfail.md */
#include "macport.h"
#include "burger.h"
#include "shipobj.h"
#include "stellar.h"

EXTERN_C_BEGIN

#define kAssertAlertID 128

typedef unsigned char Str15[16];

typedef void (*CleanUpCallBackPtr)(void);

CleanUpCallBackPtr gSWCleanUpCallBackP;              /* 0x008700d0 */

// FUNCTION: EVNOVA 0x004794d0
// MAC: _SWAssertFail i386 0x000a7a50 ppc 0x000b2850
void SWAssertFail(char *filename, int lineNum)
{
    Str255 fileString = "\p";
    Str15 lineString;
    short n, result;
    uint32_t junkTime;

    FadeScreenIn();

    NumToString(lineNum, lineString);

    for (n = 0; filename[n] != 0; n++) {
        fileString[n + 1] = filename[n];
    }
    fileString[0] += n;

    if (gSWCleanUpCallBackP != NULL)
        (*gSWCleanUpCallBackP)();

    OSCursorReset();
    ParamText("\pAssertion failure! Nova will now quit. The failure occurred here:",
              fileString, lineString, "\pN/A");
    result = StopAlert(kAssertAlertID, NULL);

    if (result < 0) {
        SysBeep(1);
        Delay(15, &junkTime);
    }

    ExitToShell();
}

EXTERN_C_END
