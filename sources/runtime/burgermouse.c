// COMPILE: -lang c++ -bool off
/* burgermouse.c -- BurgerLib InMouseWin95: mouse presence, buttons and position. Notes: Research/notes/burgermouse.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

Word MousePresent;                               /* 0x008701b8 */

/* MouseInit (BurgerLib InMouseWin95). */
// FUNCTION: EVNOVA 0x004d7af0
unsigned int MouseInit(void)
{
    MousePresent = 1;
    return 1;
}

/* MouseDestroy (BurgerLib MouseDestroy). */
// FUNCTION: EVNOVA 0x004d7b00
void MouseDestroy(void)
{
}

/* MouseReadButtons (BurgerLib InMouseWin95). */
// FUNCTION: EVNOVA 0x004d7b10
int MouseReadButtons(void)
{
    return Win95MouseButton;
}

/* Both axes are clamped to [0, extent-1]; a negative reads as 0. */
/* MouseReadPosition (BurgerLib MouseReadAbs). */
// FUNCTION: EVNOVA 0x004d7b20
void MouseReadPosition(int32_t *h, int32_t *v)
{
    int32_t p;

    if (h != 0) {
        p = Win95MouseX;
        if (p < 0)
            p = 0;
        if (p >= (int32_t)ScreenWidth)
            p = ScreenWidth - 1;
        *h = p;
    }
    if (v != 0) {
        p = Win95MouseY;
        if (p < 0)
            p = 0;
        if (p >= (int32_t)ScreenHeight)
            p = ScreenHeight - 1;
        *v = p;
    }
}

#ifdef __cplusplus
}
#endif
