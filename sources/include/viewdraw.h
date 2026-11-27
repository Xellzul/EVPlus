/* viewdraw.h -- the space view drawn by the GPU: each frame's draws built from the sprite world, the beams and
   the particles, handed to the platform with the overlays. */
#pragma once

#include <vector>
#include "macport.h"
#include "platform.h"

struct SpriteBlend;

namespace ViewDraw {

void Install();                     /* hooks the sprite world to draw through this */
void Clear();                       /* the view plain, the system's colour, no flash (landed, a refresh, dropped) */

/* Added to the frame being built: an overlay frame (comm line, floating map, escort menu) added over the view
   at `place' on the screen from its GPU target, faded by `level' of 32 (after BeginOverlays, where a frame's
   overlays start). */
void BeginOverlays();
void Overlay(SWFrameRec *frame, const Rect &place, short level);
void Flash(const RGBColor &color);  /* the whole view one colour for 50 ms of frames from the next taken (jump flashes) */

/* The frame built last: draws in view pixels; overlays, then the interface layer's sprites, in screen pixels.
   The platform shows the finished frame through the brightness, the white-out over the overlays too. */
void Take(PlatVideoView &view);

void FrameGone(SWFrameRec *frame);  /* a sprite frame is freed: its image goes with it */

/* the draws that show a sprite's image at `place' as a blend says (never the original blend), added to `list' */
void BlendDraws(PlatImage *image, const SpriteBlend &blend, const Rect &place, std::vector<PlatDraw> &list);

} // namespace ViewDraw
