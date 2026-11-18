// COMPILE: -lang c++ -bool off
/* burgergraphics.c -- BurgerLib GrGraphicsManager: the display-mode record. Notes: Research/notes/burgergraphics.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t ScreenFlags;                               /* 0x0086ff9c, the mode flags */

/* VideoGetCurrentMode (BurgerLib VideoGetCurrentMode). */
// FUNCTION: EVNOVA 0x004f9910
void VideoGetCurrentMode(int32_t *out)
{
    out[0] = ScreenWidth;
    out[1] = ScreenHeight;
    out[2] = VideoColorDepth;
    out[3] = ScreenFlags;
}

/* Four fields, three of which must be non-null before the call is worth
   making; a rejected record answers 1. */
/* VideoSetMode (BurgerLib VideoSetMode). */
// FUNCTION: EVNOVA 0x004f9940
int32_t VideoSetMode(int32_t *rec)
{
    if (rec == 0 || rec[0] == 0 || rec[1] == 0)
        return 1;
    return SetDisplayToSize(rec[0], rec[1], rec[2], rec[3]);
}

#ifdef __cplusplus
}
#endif
