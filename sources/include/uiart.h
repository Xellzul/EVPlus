/* uiart.h -- the interface's art as GPU images: pictures, icons, patterns and sprite frames as premultiplied
   ARGB with an alpha, kept while the art lives. */
#pragma once

#include <vector>
#include "macport.h"
#include "platform.h"

namespace UI {

namespace Art {

/* a picture (PICT) at width x height: a palette one stretched to the nearest pixel, a direct-colour one bicubic */
PlatImage *Picture(const Image_t *pic, int width, int height);

/* an icon (cicn) at width x height, its colour 0 clear, stretched to the nearest pixel */
PlatImage *Icon(const Image_t *icon, int width, int height);

/* a pattern (ppat) at its own size, for tiling */
PlatImage *Pattern(const Image_t *pattern);

/* a sprite frame at its own size: its RLE shape, else its pixels where its mask is black; the one image of a
   frame, the space view's too */
PlatImage *Sprite(const SWFrameRec *frame);
PlatImage *SpriteMade(const SWFrameRec *frame);     /* that image when it was made already, else NULL */

/* a picture's, icon's or pattern's pixels at its own size, Width x Height of them; keyZero: colour 0 clear */
bool Pixels(const Image_t *art, bool keyZero, std::vector<uint32_t> &out);

/* a picture's pixels at width x height, stretched as Picture stretches it */
bool PicturePixels(const Image_t *pic, int width, int height, std::vector<uint32_t> &out);

/* `part' of width x height pixels stretched to toWidth x toHeight with the bicubic kernel, alpha with colour */
void Stretch(const uint32_t *pixels, int width, int height, const Rect &part, uint32_t *out, int toWidth,
             int toHeight);

void Gone(const Image_t *art);              /* the art is released: its images go */
void FrameGone(const SWFrameRec *frame);    /* likewise a sprite frame */
void Clear();                               /* every image kept, but the sprite frames' */

} // namespace Art

} // namespace UI
