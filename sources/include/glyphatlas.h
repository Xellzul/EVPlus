/* glyphatlas.h -- the glyphs of the interface's text as parts of GPU images, each kept once: white times its
   coverage, a clear pixel around it. */
#pragma once

#include "fontraster.h"
#include "platform.h"

namespace UI {

namespace GlyphAtlas {

/* a glyph's pixels in the atlas: its width x height from x, y of the page */
struct Place {
    PlatImage *page;
    int x;
    int y;
};

/* where the glyph lies, put there when it is new; false for one without pixels, or when no page takes it */
bool Find(const RasterGlyph &glyph, Place *place);

/* every page freed, once the draws waiting are sent */
void Clear();

} // namespace GlyphAtlas

} // namespace UI
