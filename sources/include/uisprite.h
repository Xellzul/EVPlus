/* uisprite.h -- sprites drawn into the interface: the main-menu buttons, their rollover glow, the planet in the
   hail dialog. */
#pragma once

#include "macport.h"

namespace UI {

/* Draws a sprite frame with its draw proc (BlitPixieRLEDrawProc, BlitPixieRLETranslucentDrawProc, ...). On a
   scaled frame the sprite is drawn at its own size twice, over black and over white: that gives each pixel its
   colour and how much of what is under it shows through. Both are stretched to the device size and laid over
   the frame, so outlines and translucency stay smooth. */
void DrawSprite(DrawProcPtr draw, SWFrameRec *sprite, SWFrameRec *dst, Rect *srcRect, Rect *dstRect);

} // namespace UI
