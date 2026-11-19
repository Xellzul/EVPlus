/* burgeroscursor.cpp -- BurgerLib OCWin95Code: showing, hiding and resetting the system cursor. Notes: Research/notes/burgeroscursor.md */

#include "burgertypes.h"
#include "burger.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* OSCursorShow (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a20
void OSCursorShow(void)
{
    if (OSCursorVisibleFlag == 0) {
        OSCursorVisibleFlag = 1;
        PlatCursorShow(1);
    }
}

/* OSCursorHide (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a50
void OSCursorHide(void)
{
    if (OSCursorVisibleFlag != 0) {
        OSCursorVisibleFlag = 0;
        PlatCursorShow(0);
    }
}

/* OSCursorReset (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a80
void OSCursorReset(void)
{
    PlatCursorArrow();
    OSCursorVisibleFlag = 1;
    PlatCursorShow(1);
}

#ifdef __cplusplus
}
#endif
