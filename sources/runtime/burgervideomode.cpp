/* burgervideomode.cpp -- BurgerLib GrWin95Code on the platform layer (platform.h): the display mode, the flip and the release of the screen (screen.h). DirectDraw's surfaces, palette and gamma ramp are gone. Notes: Research/notes/burgervideomode.md */

#include "burger.h"
#include "platform.h"
#include "screen.h"

#define kFullscreen      1                      /* SETDISPLAYFULLSCREEN */
#define kDepth555        15

/* UpdateAndPageFlip (BurgerLib GrWin95Code): show the screen now. */
// FUNCTION: EVNOVA 0x004ec960
void UpdateAndPageFlip(void)
{
    Screen::Changed();
    Screen::EndFrame();
}

/* ReleaseVideo (BurgerLib GrWin95Code). */
// FUNCTION: EVNOVA 0x004ec990
void ReleaseVideo(void)
{
    PlatVideoClose();
    Screen::Close();
}

/* SetDisplayToSize (BurgerLib SetDisplayToSize). The one mode there is: a screen of the size asked for (depth
   15, as the game asks), shown full screen or in a window; the refresh rate in depth's high word is not used. */
// FUNCTION: EVNOVA 0x004ecbd0
int32_t SetDisplayToSize(ULong width, ULong height, ULong depth, ULong flags)
{
    depth = (unsigned short)depth;
    ReleaseVideo();
    if (depth != kDepth555
        || PlatVideoOpen(width, height, (flags & kFullscreen) != 0) != 0) {
        NonFatal("Couldn't set display mode %dx%dx%d\n", width, height, depth);
        return 1;
    }
    if (!Screen::Open(width, height)) {
        PlatVideoClose();
        return 1;
    }
    return 0;
}
