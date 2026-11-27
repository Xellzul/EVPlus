/* qdpen.cpp -- the pen and the shapes drawn with it: MoveTo, LineTo, PenSize, FrameOval, PaintOval. */
#include "scaleddraw.h"

/* The pen position and size live in the current port, so these are field stores with
   the Toolbox's null guard. */

// FUNCTION: EVNOVA 0x004b97e0
void LineTo(short h, short v)
{
    if (CurrentGrafPort == NULL)
        return;
    if (CurrentGrafPort->pix.pixelSize == 16)
        Scaled::LineTo(h, v);
    else
        MoveTo(h, v);
}

// FUNCTION: EVNOVA 0x004ba2f0
void MoveTo(short h, short v)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->pnLoc.h = h;
    CurrentGrafPort->pnLoc.v = v;
    CurrentGrafPort->pnDeviceOffset = 0;
}

#if EVPLUS_TEST_PAGES
// FUNCTION: EVNOVA 0x004ba320
void PenSize(short width, short height)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->pnSize.h = width;
    CurrentGrafPort->pnSize.v = height;
}

// FUNCTION: EVNOVA 0x004ba350
void FrameOval(Rect *r)
{
    if (CurrentGrafPort != NULL && r != NULL && CurrentGrafPort->pix.pixelSize == 16)
        Scaled::FrameOval(*r);
}

// FUNCTION: EVNOVA 0x004bb260
void PaintOval(Rect *r)
{
    if (CurrentGrafPort != NULL && r != NULL && CurrentGrafPort->pix.pixelSize == 16)
        Scaled::PaintOval(*r);
}
#endif
