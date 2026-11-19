/* rectfill.cpp -- FillRectWithColor (win 0x004b6a10): clip a Rect to the current port and fill it with an RGBColor. Notes: Research/notes/rectfill.md */
#include <stddef.h>

#include "macport.h"
#include "scaleddraw.h"

CGrafPtr CurrentGrafPort;             /* 0x00870064 */

/* A rect filled with a colour on the current port (16-bit; nothing on other depths, as the exe). */
// FUNCTION: EVNOVA 0x004b6a10
void FillRectWithColor(Rect *r, RGBColor *color)
{
    if (r != NULL && CurrentGrafPort->pix.pixelSize == 16)
        Scaled::FillRect(*r, *color);
}
