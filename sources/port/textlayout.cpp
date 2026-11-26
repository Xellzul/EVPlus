/* textlayout.cpp -- text layout without a port (textlayout.h), with GDI DrawText's rules as far as the game
   meets them:
   - a line's first pixel row is the rectangle's top; the baseline lies the font's ascent below it; a wrapped
     line takes ascent + descent;
   - with word wrap a line ends at CR, LF or CR LF, or before the word that no longer fits, at the last
     space; a word wider than the line is not cut;
   - measured without DT_NOPREFIX an `&' is an accelerator mark and has no width (`&&' is one `&').
   Glyphs are placed at the device size, each one its hinted advance after the one before it, in whole device
   pixels; a width is the sum of those advances in logical pixels, rounded once. */

#include <vector>
#include "textlayout.h"

#define kSpace          ' '
#define kReturn         '\r'
#define kLineFeed       '\n'
#define kPrefix         '&'
#define kFirstVisible   0x20

/* Mac OS Roman 0x80..0xff */
static const unsigned short kMacRoman[128] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8,
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
    0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

/* Windows code page 1252, 0x80..0x9f; the rest of it is Latin-1 */
static const unsigned short kAnsiHigh[32] = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
};

/* a character's Mac Roman code, 0 when Mac Roman does not have it */
static unsigned char ToMacRoman(unsigned unicode)
{
    int i;

    if (unicode < 0x80)
        return (unsigned char)unicode;
    for (i = 0; i < 128; i++) {
        if (kMacRoman[i] == unicode)
            return (unsigned char)(0x80 + i);
    }
    return 0;
}

/* a byte of code page 1252 as a Unicode character */
unsigned AnsiToUnicode(unsigned char c)
{
    return (c >= 0x80 && c < 0xa0) ? kAnsiHigh[c - 0x80] : c;
}

namespace TextLayout {

int FromMacRoman(const char *text, int count, Unit *out)
{
    unsigned char c;
    int i;

    for (i = 0; i < count; i++) {
        c = (unsigned char)text[i];
        out[i].unicode = c < 0x80 ? c : kMacRoman[c - 0x80];
        out[i].macRoman = c;
    }
    return count;
}

/* the ANSI code page is 1252 here, where Windows used the system's */
int FromAnsi(const char *text, int count, Unit *out)
{
    unsigned char c;
    int n;
    int i;

    n = 0;
    for (i = 0; i < count; i++) {
        c = (unsigned char)text[i];
        if (c == kPrefix) {
            if (i + 1 >= count || (unsigned char)text[i + 1] != kPrefix)
                continue;
            i++;
        }
        out[n].unicode = AnsiToUnicode(c);
        out[n].macRoman = ToMacRoman(out[n].unicode);
        n++;
    }
    return n;
}

short DeviceSize(short size, double scale)
{
    if (scale == 1.0)
        return size;
    if (size < kMinFontSize)
        size = kMinFontSize;
    return (short)(size * scale + 0.5);
}

const RasterGlyph *GlyphOf(const FontSpec &spec, const Unit &unit)
{
    if (unit.unicode < kFirstVisible)
        return NULL;                    /* a control character has no shape and no width */
    return RasterGetGlyph(spec.font, spec.pixelSize, spec.flags, unit.unicode, unit.macRoman);
}

int Width(const FontSpec &spec, const Unit *units, int count)
{
    const RasterGlyph *glyph;
    int width;
    int i;

    width = 0;
    for (i = 0; i < count; i++) {
        glyph = GlyphOf(spec, units[i]);
        if (glyph != NULL)
            width += glyph->advance;
    }
    return width;
}

bool NextLine(const UI::Mapping &map, const FontSpec &spec, const Unit *units, int count, int start, int maxWidth,
              int *end, int *next)
{
    const RasterGlyph *glyph;
    int lastSpace;
    int width;
    int c;
    int i;

    if (start >= count)
        return false;
    lastSpace = -1;
    width = 0;
    for (i = start; i < count; i++) {
        c = units[i].unicode;
        if (c == kReturn || c == kLineFeed) {
            *end = i;
            *next = (c == kReturn && i + 1 < count && units[i + 1].unicode == kLineFeed) ? i + 2 : i + 1;
            return true;
        }
        glyph = GlyphOf(spec, units[i]);
        if (c == kSpace) {
            lastSpace = i;
        } else if (glyph != NULL && map.LogicalLength(width + glyph->advance) > maxWidth && lastSpace >= 0) {
            *end = lastSpace;
            *next = lastSpace + 1;
            return true;
        }
        if (glyph != NULL)
            width += glyph->advance;
    }
    *end = count;
    *next = count;
    return true;
}

/* characters from device x on one baseline, each glyph its advance after the last; returns the x after them */
static int Run(Sink *sink, const FontSpec &spec, const Unit *units, int count, int x, int baseline,
               const Clip &clip, const RGBColor *colour)
{
    const RasterGlyph *glyph;
    int pen;
    int i;

    pen = x;
    for (i = 0; i < count; i++) {
        glyph = GlyphOf(spec, units[i]);
        if (glyph == NULL)
            continue;
        if (sink != NULL)
            sink->Glyph(*glyph, pen, baseline, colour, clip);
        pen += glyph->advance;
    }
    return pen;
}

/* wrapped lines into `box' (logical, the surface's origin applied); returns the height of all lines */
static int Wrapped(Sink *sink, const UI::Mapping &map, const Setup &setup, const Unit *units, int count,
                   const Rect &box, int scroll, const Clip &clip, const TextSpan *spans, int spanCount)
{
    const RGBColor *runColour;
    int baseline;
    int drop;
    int start;
    int end;
    int next;
    int pos;
    int stop;
    int pen;
    int span;

    /* lines sit at fixed device offsets from the first baseline, so scrolled text moves as one */
    baseline = map.Y(box.top - scroll + setup.ascent);
    drop = 0;
    start = 0;
    span = 0;
    while (NextLine(map, setup.glyphs, units, count, start, box.right - box.left, &end, &next)) {
        pen = map.X(box.left);
        pos = start;
        while (pos < end) {
            while (span < spanCount && (int)spans[span].end <= pos)
                span++;
            if (span < spanCount && (int)spans[span].start <= pos) {
                stop = (int)spans[span].end < end ? (int)spans[span].end : end;
                runColour = &spans[span].color;
            } else {
                stop = span < spanCount && (int)spans[span].start < end ? (int)spans[span].start : end;
                runColour = NULL;
            }
            pen = Run(sink, setup.glyphs, units + pos, stop - pos, pen, baseline + map.DeviceLengthY(drop), clip,
                      runColour);
            pos = stop;
        }
        drop += setup.ascent + setup.descent;
        start = next;
    }
    return drop;
}

int Draw(Sink *sink, const UI::Mapping &map, const Setup &setup, const void *text, uint32_t byteCount,
         const Rect *box, bool wrapped, int scroll, const TextSpan *spans, int spanCount, Pen &pen)
{
    Clip clip;
    Rect logicalBox;
    Rect deviceBox;
    int x;
    int y;
    int end;

    if (text == NULL || byteCount == 0 || (wrapped && box == NULL))
        return 0;
    std::vector<Unit> units(byteCount);
    FromMacRoman((const char *)text, byteCount, units.data());

    clip.left = 0;
    clip.top = 0;
    clip.right = setup.width;
    clip.bottom = setup.height;
    if (box != NULL) {
        /* a box is in the surface's own pixels, its origin not applied */
        logicalBox = *box;
        OffsetRect(&logicalBox, setup.left, setup.top);
        deviceBox = map.DeviceRect(logicalBox);
        if (clip.right > deviceBox.right)
            clip.right = deviceBox.right;
        if (clip.bottom > deviceBox.bottom)
            clip.bottom = deviceBox.bottom;
    }
    if (!wrapped) {
        /* from the pen's device x, kept to the box right of and under the pen; the pen ends after the text */
        x = map.X(pen.h) + pen.deviceOffset;
        y = pen.v - setup.size;
        if (box != NULL) {
            if (clip.left < x)
                clip.left = x;
            if (clip.top < map.Y(y))
                clip.top = map.Y(y);
        }
        end = Run(sink, setup.glyphs, units.data(), byteCount, x, map.Y(y + setup.ascent), clip, NULL);
        pen.h = ClampShort(pen.h + map.LogicalLength(end - map.X(pen.h)));
        pen.deviceOffset = end - map.X(pen.h);
        return 0;
    }
    /* into the box, the first `scroll' rows above its top */
    if (clip.left < deviceBox.left)
        clip.left = deviceBox.left;
    if (clip.top < deviceBox.top)
        clip.top = deviceBox.top;
    return Wrapped(sink, map, setup, units.data(), byteCount, logicalBox, scroll, clip, spans, spanCount);
}

} // namespace TextLayout
