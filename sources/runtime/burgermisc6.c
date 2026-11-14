// COMPILE: -lang c++ -bool off
/* burgermisc6.c -- burgermisc, exe run 6. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

struct joyinfoex_tag {
    Word32 dwSize;
    Word32 dwFlags;
    Word32 dwXpos;
    Word32 dwYpos;
    Word32 dwZpos;
    Word32 dwRpos;
    Word32 dwUpos;
    Word32 dwVpos;
    Word32 dwButtons;
    Word32 dwButtonNumber;
    Word32 dwPOV;
    Word32 dwReserved1;
    Word32 dwReserved2;
};

/* The member's 0x38 bytes of .bss; no live code reads them. */
static joyinfoex_tag PrivJoyInfo;
static Word PrivJoyNumber;

#ifdef __cplusplus
extern "C" {
#endif

/* JoystickDestroy (BurgerLib InJoystickWin95 JoystickDestroy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00502490
void JoystickDestroy(void)
{
    JoystickPresent = 0;
}

#ifdef __cplusplus
}
#endif
