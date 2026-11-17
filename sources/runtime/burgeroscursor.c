// COMPILE: -lang c++ -bool off
/* burgeroscursor.c -- BurgerLib OCWin95Code: showing, hiding and resetting the system cursor. Notes: Research/notes/burgeroscursor.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

Word OSCursorLastCursor;                         /* 0x00870dc8 */

/* OSCursorShow (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a20
void OSCursorShow(void)
{
    if (OSCursorVisibleFlag == 0) {
        OSCursorVisibleFlag = 1;
        while (ShowCursor(1) < 0)
            ;
    }
}

/* OSCursorHide (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a50
void OSCursorHide(void)
{
    if (OSCursorVisibleFlag != 0) {
        OSCursorVisibleFlag = 0;
        while (ShowCursor(0) >= 0)
            ;
    }
}

/* OSCursorReset (BurgerLib OCWin95Code). */
// FUNCTION: EVNOVA 0x004f1a80
void OSCursorReset(void)
{
    void *cursor;

    if (OSCursorLastCursor != IDC_ARROW) {
        cursor = LoadCursorA(Burger::Win32::m_pInstance,(const char *)IDC_ARROW);
        if (cursor != 0 || (cursor = LoadCursorA(0, (const char *)IDC_ARROW)) != 0) {
            OSCursorLastCursor = IDC_ARROW;
            SetCursor(cursor);
        }
    }
    OSCursorVisibleFlag = 1;
    while (ShowCursor(1) < 0)
        ;
}

#ifdef __cplusplus
}
#endif
