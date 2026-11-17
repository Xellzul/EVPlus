// COMPILE: -lang c++ -bool off
/* burgermisc2.c -- burgermisc, exe run 2. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char OSCursorVisibleFlag = 1;           /* 0x00577a34 */

/* OSCursorIsVisible (BurgerLib OCOSCursorManager's: returns the flag, 1 after OSCursorShow). */
// FUNCTION: EVNOVA 0x004f1ca0
unsigned int OSCursorIsVisible(void)
{
    return OSCursorVisibleFlag;
}

#ifdef __cplusplus
}
#endif
