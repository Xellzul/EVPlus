/* textlayout.h -- text laid out for drawing and measuring (port/textlayout.cpp): characters, line breaks, spans
   and each glyph's device place, handed to a sink. It knows no port: the font, the surface's size and the pen
   come with the call. */
#pragma once

#include "fontraster.h"
#include "uiscale.h"

namespace TextLayout {

/* a character: what the font is asked for */
struct Unit {
    unsigned short unicode;
    unsigned char macRoman;             /* its Mac Roman code, 0 when it has none */
};

/* the bytes as the characters drawn (Mac Roman), or as measured (code page 1252, `&' a mark); returns how many */
int FromMacRoman(const char *text, int count, Unit *out);
int FromAnsi(const char *text, int count, Unit *out);

/* the device pixels a text is kept to */
struct Clip {
    int left;
    int top;
    int right;
    int bottom;
};

/* takes the glyphs of a text as they are placed */
class Sink {
public:
    /* the glyph's origin at device x on the baseline; colour NULL: the text's own, else its span's */
    virtual void Glyph(const RasterGlyph &glyph, int x, int baseline, const RGBColor *colour, const Clip &clip) = 0;

protected:
    ~Sink() = default;
};

/* where text starts: the logical pen, and how far the device end of the text before it lies past it */
struct Pen {
    short h;
    short v;
    int32_t deviceOffset;
};

/* what a text is drawn with and on */
struct Setup {
    FontSpec glyphs;                    /* the font at the device size */
    int ascent;                         /* its rows at the logical size */
    int descent;
    short size;                         /* the size asked for: the pen's baseline lies the ascent under v - size */
    int width;                          /* the surface, in device pixels */
    int height;
    short left;                         /* the surface's logical origin */
    short top;
};

/* a logical coordinate kept to a short's range */
inline short ClampShort(int value)
{
    return (short)(value < -32768 ? -32768 : value > 32767 ? 32767 : value);
}

/* a font size on a surface scaled by `scale': the smallest size applied first, so text keeps its proportions */
short DeviceSize(short size, double scale);

/* a character's glyph, NULL for a control character or when none can be made */
const RasterGlyph *GlyphOf(const FontSpec &spec, const Unit &unit);

/* the device advances of the characters' glyphs */
int Width(const FontSpec &spec, const Unit *units, int count);

/* one wrapped line from `start' on: its characters end before `end', the next begins at `next'; false = none left */
bool NextLine(const UI::Mapping &map, const FontSpec &spec, const Unit *units, int count, int start, int maxWidth,
              int *end, int *next);

/* Mac Roman text through the sink (NULL: measured only): from the pen, or wrapped into box (returns its height) */
int Draw(Sink *sink, const UI::Mapping &map, const Setup &setup, const void *text, uint32_t byteCount,
         const Rect *box, bool wrapped, int scroll, const TextSpan *spans, int spanCount, Pen &pen);

} // namespace TextLayout

/* the current port's font, surface and pen as Draw takes them (port/qdtext.cpp); 0 when it has no port or font */
int PortTextSetup(TextLayout::Setup *setup, TextLayout::Pen *pen);
