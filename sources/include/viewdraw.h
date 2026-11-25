/* viewdraw.h -- the space view drawn by the GPU: each frame's draws built from the sprite world, the beams and
   the particles, handed to the platform with the overlays. */
#pragma once

#include "macport.h"
#include "platform.h"

namespace ViewDraw {

void Install();                     /* hooks the sprite world to draw through this */
void Clear();                       /* the view plain, the system's colour, no flash (landed, a refresh, dropped) */

/* Added to the frame being built: an overlay frame (comm line, floating map, escort menu) laid over the view
   at `place' on the page, faded by `level' of 32 (after BeginOverlays, where the page's overlays start). */
void BeginOverlays();
void Overlay(SWFrameRec *frame, const Rect &place, short level);
void Flash(const RGBColor &color);  /* the whole view one colour for 50 ms of frames from the next taken (jump flashes) */

/* The frame built last: draws in view pixels; overlays, then the interface layer's sprites, in page pixels.
   Brightness is applied to each image and colour before blending (exact at the default brightness; with the
   original blending, to the whole view after it), and overlays are laid over the white-out. */
void Take(PlatVideoView &view);

void FrameGone(SWFrameRec *frame);  /* a sprite frame is freed: its image goes with it */

} // namespace ViewDraw
