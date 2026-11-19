/* paintrect.cpp -- PaintRect, InvertRect, EraseRect, FrameRect; exe 0x004b6b90..0x004b6ef0. Notes: Research/notes/qddraw.md */

#include <stddef.h>
#include "macport.h"
#include "burger.h"
#include "render.h"
#include "scaleddraw.h"

// FUNCTION: EVNOVA 0x004b6b90
void PaintRect(Rect *r)
{
    if (CurrentGrafPort == NULL)
        return;
    FillRectWithColor(r, &CurrentGrafPort->foreColor);
}

/* the colours of a rect inverted (16-bit; every port the game makes is) */
// FUNCTION: EVNOVA 0x004b6bc0
void InvertRect(Rect *r)
{
    if (CurrentGrafPort != NULL && r != NULL && CurrentGrafPort->pix.pixelSize == 16)
        Scaled::InvertRect(*r);
}

/* the Toolbox erase is a fill in the port's background colour */
// FUNCTION: EVNOVA 0x004b6ec0
void EraseRect(Rect *r)
{
    if (CurrentGrafPort == NULL)
        return;
    FillRectWithColor(r, &CurrentGrafPort->backColor);
}

/* one pixel wide, whatever the pen (as the exe); 16-bit only */
// FUNCTION: EVNOVA 0x004b6ef0
void FrameRect(const Rect *r)
{
    if (CurrentGrafPort == NULL || r == NULL || CurrentGrafPort->pix.pixelSize != 16)
        return;
    Scaled::FrameRect(*r, CurrentGrafPort->foreColor);
}
