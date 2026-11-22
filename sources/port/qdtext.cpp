/* qdtext.cpp -- the port's QuickDraw text calls, DrawTextRun .. MeasureWrappedText; exe 0x004bc760..0x004bcc70.
   The exe has GDI's DrawText draw, measure and wrap; the same is done here on port/fontraster.cpp, with
   DrawText's rules as far as the game meets them:
   - a line's first pixel row is the rectangle's top; the baseline lies the font's ascent below it; a wrapped
     line takes ascent + descent;
   - with word wrap a line ends at CR, LF or CR LF, or before the word that no longer fits, at the last
     space; a word wider than the line is not cut;
   - measured without DT_NOPREFIX an `&' is an accelerator mark and has no width (`&&' is one `&').
   Glyphs are drawn at the port's device size, each one its hinted advance after the one before it, in whole
   device pixels; a width is the sum of those advances in logical pixels, rounded once.
   Notes: Research/notes/toolbox.md */

#include "burger.h"
#include "fontraster.h"
#include "pixel.h"
#include "uiscale.h"

#define kSpace          ' '
#define kReturn         '\r'
#define kLineFeed       '\n'
#define kPrefix         '&'
#define kFirstVisible   0x20
#define kFull           255
#define kFallbackFont   0               /* the system font's number */

/* a character: what the font is asked for */
typedef struct TextUnit {
    unsigned short unicode;
    unsigned char macRoman;             /* its Mac Roman code, 0 when it has none */
} TextUnit;

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

/* Mac Roman bytes as characters, as DrawTextRun draws them */
static int FromMacRoman(const char *text, int count, TextUnit *out)
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

/* The same bytes as the exe's measuring calls take them: in the ANSI code page (DrawTextA; code page 1252
   here, where Windows used the system's), and with `&' as the accelerator mark. Returns how many characters
   are left. */
static int FromAnsi(const char *text, int count, TextUnit *out)
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

/* -- the port's font ----------------------------------------------------------------------------------------- */

typedef struct TextStyle {
    FontSpec spec;
    int ascent;
    int descent;
} TextStyle;

/* A size as asked for, or, for the glyphs drawn on a scaled port, scaled (after the smallest size is applied
   to it, so text keeps its proportions). */
static short SizeFor(short size, double scale)
{
    if (scale == 1.0)
        return size;
    if (size < kMinFontSize)
        size = kMinFontSize;
    return (short)(size * scale + 0.5);
}

/* The font, size and style set in the port, scaled by `scale': 1 gives the port's own size (rows: ascent and
   descent), the port's device scale the glyphs drawn and measured. A scaled font never uses a bitmap strike,
   so its glyphs are the same outlines at every scale. A font number the font cache does not know (the
   scrolling text asks for 20, the Mac's number for Times) leaves the port with the font it drew or measured
   with before; a port that has had none gets font 0 at the port's size. */
static int FontAt(FontSpec *spec, double scale)
{
    CGrafPtr port;

    port = CurrentGrafPort;
    if (GetFont(port->txFont, SizeFor(port->txSize, scale), port->txFace, spec)) {
        port->lastFont = port->txFont;
        port->lastSize = port->txSize;
        port->lastFace = port->txFace;
        port->hasLastFont = 1;
    } else if (!(port->hasLastFont && GetFont(port->lastFont, SizeFor(port->lastSize, scale), port->lastFace, spec))
               && !GetFont(kFallbackFont, SizeFor(port->txSize, scale), port->txFace, spec)) {
        return 0;
    }
    if (scale != 1.0)
        spec->flags |= kRasterNoStrike;
    return 1;
}

/* the port's font at its own size, with its ascent and descent */
static int CurrentStyle(TextStyle *style)
{
    if (!FontAt(&style->spec, 1.0))
        return 0;
    RasterMetrics(style->spec.font, style->spec.pixelSize, &style->ascent, &style->descent);
    return 1;
}

static const RasterGlyph *GlyphOf(const FontSpec *spec, const TextUnit *unit)
{
    if (unit->unicode < kFirstVisible)
        return NULL;                    /* a control character has no shape and no width */
    return RasterGetGlyph(spec->font, spec->pixelSize, spec->flags, unit->unicode, unit->macRoman);
}

static int WidthOf(const FontSpec *spec, const TextUnit *units, int count)
{
    const RasterGlyph *glyph;
    int width;
    int i;

    width = 0;
    for (i = 0; i < count; i++) {
        glyph = GlyphOf(spec, &units[i]);
        if (glyph != NULL)
            width += glyph->advance;
    }
    return width;
}

static short ClampShort(int value)
{
    return (short)(value < -32768 ? -32768 : value > 32767 ? 32767 : value);
}

/* One line of word-wrapped text from `start' on, measured with the glyphs drawn on `map': its characters end
   before `end', the next line begins at `next'. 0 = no line left. */
static int NextLine(const UI::Mapping &map, const FontSpec *spec, const TextUnit *units, int count, int start,
                    int maxWidth, int *end, int *next)
{
    const RasterGlyph *glyph;
    int lastSpace;
    int width;
    int c;
    int i;

    if (start >= count)
        return 0;
    lastSpace = -1;
    width = 0;
    for (i = start; i < count; i++) {
        c = units[i].unicode;
        if (c == kReturn || c == kLineFeed) {
            *end = i;
            *next = (c == kReturn && i + 1 < count && units[i + 1].unicode == kLineFeed) ? i + 2 : i + 1;
            return 1;
        }
        glyph = GlyphOf(spec, &units[i]);
        if (c == kSpace) {
            lastSpace = i;
        } else if (glyph != NULL && map.LogicalLength(width + glyph->advance) > maxWidth && lastSpace >= 0) {
            *end = lastSpace;
            *next = lastSpace + 1;
            return 1;
        }
        if (glyph != NULL)
            width += glyph->advance;
    }
    *end = count;
    *next = count;
    return 1;
}

/* -- drawing ------------------------------------------------------------------------------------------------- */

typedef struct ClipRect {
    int left;
    int top;
    int right;
    int bottom;
} ClipRect;

/* `old' moved towards `target' by coverage `a' of kFull, to the nearest step either way */
static int Blend(int old, int target, int a)
{
    int moved;

    moved = (target - old) * a;
    return old + (moved >= 0 ? moved + kFull / 2 : moved - kFull / 2) / kFull;
}

/* a text colour through the gamma ramp, a channel in 8 bits, as the exe hands it to GDI */
typedef struct TextColor {
    int r;
    int g;
    int b;
} TextColor;

static void DrawGlyph(PixMap *pix, const RasterGlyph *glyph, int x, int baseline, const ClipRect *clip,
                      const TextColor *color)
{
    const unsigned char *cover;
    unsigned char *p;
    unsigned short old;
    int left;
    int top;
    int cx;
    int cy;
    int px;
    int py;
    int a;
    int r;
    int g;
    int b;

    left = x + glyph->left;
    top = baseline - glyph->top;
    for (cy = 0; cy < glyph->height; cy++) {
        py = top + cy;
        if (py < clip->top || py >= clip->bottom)
            continue;
        cover = glyph->coverage + cy * glyph->width;
        for (cx = 0; cx < glyph->width; cx++) {
            a = cover[cx];
            px = left + cx;
            if (a == 0 || px < clip->left || px >= clip->right)
                continue;
            p = (unsigned char *)pix->baseAddr + py * pix->rowBytes + px * (pix->pixelSize / 8);
            if (pix->pixelSize == 16) {
                r = color->r >> 3;
                g = color->g >> 3;
                b = color->b >> 3;
                if (a != kFull) {
                    old = *(unsigned short *)p;
                    r = Blend(Pixel::Red(old), r, a);
                    g = Blend(Pixel::Green(old), g, a);
                    b = Blend(Pixel::Blue(old), b, a);
                }
                *(unsigned short *)p = Pixel::Pack(r, g, b);
            } else {
                p[0] = Blend(p[0], color->b, a);
                p[1] = Blend(p[1], color->g, a);
                p[2] = Blend(p[2], color->r, a);
            }
        }
    }
}

/* Characters from device x on, on one device baseline, each glyph its advance after the one before it.
   Returns the device x after them. */
static int DrawRun(PixMap *pix, const FontSpec *spec, const TextUnit *units, int count, int x, int baseline,
                   const ClipRect *clip, const TextColor *color)
{
    const RasterGlyph *glyph;
    int pen;
    int i;

    pen = x;
    for (i = 0; i < count; i++) {
        glyph = GlyphOf(spec, &units[i]);
        if (glyph == NULL)
            continue;
        if (pix != NULL)
            DrawGlyph(pix, glyph, pen, baseline, clip, color);
        pen += glyph->advance;
    }
    return pen;
}

/* `rgb' as a TextColor */
static TextColor GammaColor(const RGBColor &rgb)
{
    TextColor color;

    color.r = (unsigned char)(gGammaRamp[rgb.red >> 11] * 8);
    color.g = (unsigned char)(gGammaRamp[rgb.green >> 11] * 8);
    color.b = (unsigned char)(gGammaRamp[rgb.blue >> 11] * 8);
    return color;
}

/* Wrapped lines into `box' (logical, pixel-map coordinates), the first `scroll' rows above its top, each
   span in its own colour; pix NULL only measures. Returns the height of all lines. */
static int DrawWrapped(PixMap *pix, const UI::Mapping &map, const TextStyle &layout, const FontSpec *glyphs,
                       const TextUnit *units, int count, const Rect &box, int scroll, const ClipRect *clip,
                       const TextColor *color, const TextSpan *spans, int spanCount)
{
    TextColor spanColor;
    const TextColor *runColor;
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
    baseline = map.Y(box.top - scroll + layout.ascent);
    drop = 0;
    start = 0;
    span = 0;
    while (NextLine(map, glyphs, units, count, start, box.right - box.left, &end, &next)) {
        pen = map.X(box.left);
        pos = start;
        while (pos < end) {
            while (span < spanCount && (int)spans[span].end <= pos)
                span++;
            if (span < spanCount && (int)spans[span].start <= pos) {
                stop = (int)spans[span].end < end ? (int)spans[span].end : end;
                spanColor = GammaColor(spans[span].color);
                runColor = &spanColor;
            } else {
                stop = span < spanCount && (int)spans[span].start < end ? (int)spans[span].start : end;
                runColor = color;
            }
            pen = DrawRun(pix, glyphs, units + pos, stop - pos, pen, baseline + map.DeviceLengthY(drop), clip,
                          runColor);
            pos = stop;
        }
        drop += layout.ascent + layout.descent;
        start = next;
    }
    return drop;
}

/* Rectangles are in the pixel map's own pixels, the port's bounds not applied to `box', as the exe gives
   them to GDI. Rows and line pitch are in logical pixels at the port's size; widths and line breaks come from
   the device advances of the glyphs drawn. Unclipped text starts at the pen's device x and leaves the pen at
   the logical pixel nearest its end, the rest in pnDeviceOffset, so the next run continues exactly. clipped != 0
   wraps into `box' and needs one; then the height of the wrapped lines is returned. draw 0 only measures. */
static int DrawTextCore(const void *text, uint32_t byteCount, Rect *box, char clipped, int height,
                        const TextSpan *spans, int spanCount, char draw)
{
    CGrafPtr port;
    PixMap *pix;
    TextUnit *units;
    TextStyle layout;
    FontSpec glyphs;
    TextColor color;
    ClipRect clip;
    Rect logicalBox;
    Rect deviceBox;
    int x;
    int y;
    int end;
    int wrapped;

    if (text == NULL || byteCount == 0 || (clipped != 0 && box == NULL))
        return 0;
    port = CurrentGrafPort;
    if (port == NULL || !CurrentStyle(&layout))
        return 0;
    UI::Mapping map(port->pix);
    if (!FontAt(&glyphs, map.ScaleY()))
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    FromMacRoman((const char *)text, byteCount, units);

    /* the exe draws on the screen port through the window, where it does not reach the game's picture */
    pix = (port->offscreen != 0 && (port->pix.pixelSize == 16 || port->pix.pixelSize == 32)) ? &port->pix : NULL;
    if (draw == 0)
        pix = NULL;
    color = GammaColor(port->foreColor);

    clip.left = 0;
    clip.top = 0;
    clip.right = port->pix.width;
    clip.bottom = port->pix.height;
    if (box != NULL) {
        logicalBox = *box;
        OffsetRect(&logicalBox, port->pix.bounds.left, port->pix.bounds.top);
        deviceBox = map.DeviceRect(logicalBox);
        if (clip.right > deviceBox.right)
            clip.right = deviceBox.right;
        if (clip.bottom > deviceBox.bottom)
            clip.bottom = deviceBox.bottom;
    }
    wrapped = 0;
    if (clipped == 0) {
        x = map.X(port->pnLoc.h) + port->pnDeviceOffset;
        y = port->pnLoc.v - port->txSize;
        if (box != NULL) {
            if (clip.left < x)
                clip.left = x;
            if (clip.top < map.Y(y))
                clip.top = map.Y(y);
        }
        end = DrawRun(pix, &glyphs, units, byteCount, x, map.Y(y + layout.ascent), &clip, &color);
        port->pnLoc.h = ClampShort(port->pnLoc.h + map.LogicalLength(end - map.X(port->pnLoc.h)));
        port->pnDeviceOffset = end - map.X(port->pnLoc.h);
    } else {
        if (clip.left < deviceBox.left)
            clip.left = deviceBox.left;
        if (clip.top < deviceBox.top)
            clip.top = deviceBox.top;
        wrapped = DrawWrapped(pix, map, layout, &glyphs, units, byteCount, logicalBox, height, &clip, &color,
                              spans, spanCount);
    }
    DeallocAPointer(units);
    return wrapped;
}

// FUNCTION: EVNOVA 0x004bc760
void DrawTextRun(const void *text, uint32_t byteCount, Rect *box, char clipped, int height)
{
    DrawTextCore(text, byteCount, box, clipped, height, NULL, 0, 1);
}

/* DrawTextRun wrapped into `box', scrolled up by `scroll' rows, with the bytes of each span (sorted, not
   overlapping) in its colour and the rest in the foreground colour. Returns the height of all lines. */
int DrawTextSpans(const void *text, uint32_t byteCount, Rect *box, int scroll, const TextSpan *spans,
                  int spanCount)
{
    return DrawTextCore(text, byteCount, box, 1, scroll, spans, spanCount, 1);
}

/* the height DrawTextSpans wraps the text into `box' */
int MeasureTextSpans(const void *text, uint32_t byteCount, Rect *box)
{
    return DrawTextCore(text, byteCount, box, 1, 0, NULL, 0, 0);
}

// FUNCTION: EVNOVA 0x004bca70
void DrawText(const void *textBuf, int firstByte, int byteCount)
{
    DrawTextRun((char *)textBuf + firstByte, byteCount, NULL, 0, 0);
}

// FUNCTION: EVNOVA 0x004bca90
void DrawString(const unsigned char *str)
{
    DrawTextRun(str + 1, str[0], NULL, 0, 0);
}

// FUNCTION: EVNOVA 0x004bcab0
void DrawCString(const char *text)
{
    DrawTextRun(text, strlen(text), NULL, 0, 0);
}

/* -- measuring ----------------------------------------------------------------------------------------------- */

/* the device advances of the glyphs DrawTextRun draws for these characters on the current port */
static int DeviceWidthOf(const UI::Mapping &map, const TextUnit *units, int count)
{
    FontSpec glyphs;

    if (!FontAt(&glyphs, map.ScaleY()))
        return 0;
    return WidthOf(&glyphs, units, count);
}

/* BUG (exe, all Windows versions): StringWidth, MeasureCString, TextWidth and MeasureWrappedText take the
   bytes as ANSI text with `&' as an accelerator mark, while DrawTextRun draws them as Mac Roman with `&'
   shown: text with characters above 0x7f or with an `&' is measured at another width than it is drawn. */
static int AnsiDeviceWidth(const UI::Mapping &map, const char *text, int byteCount)
{
    TextUnit *units;
    int width;

    if (byteCount <= 0)
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    width = DeviceWidthOf(map, units, FromAnsi(text, byteCount, units));
    DeallocAPointer(units);
    return width;
}

static short MeasureAnsi(const char *text, int byteCount)
{
    UI::Mapping map(CurrentGrafPort->pix);

    return ClampShort(map.LogicalLength(AnsiDeviceWidth(map, text, byteCount)));
}

/* the same measurer over a Pascal string */
// FUNCTION: EVNOVA 0x004bcad0
short StringWidth(const unsigned char *str)
{
    char text[256];

    PStr2CStr(text, str);
    return MeasureCString(text);
}

// FUNCTION: EVNOVA 0x004bcb00
short MeasureCString(const char *text)
{
    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || text[0] == 0)
        return 0;
    return MeasureAnsi(text, strlen(text));
}

// FUNCTION: EVNOVA 0x004bcbc0
short TextWidth(const char *text, int byteCount)
{
    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || text[0] == 0)
        return 0;
    return MeasureAnsi(text, byteCount);
}

/* TextWidth in the current port's device pixels, before rounding to logical ones: where the glyphs of the
   first byteCount bytes end, from where they start */
int TextDeviceWidth(const char *text, int byteCount)
{
    if (CurrentGrafPort == NULL || text == NULL || text[0] == 0)
        return 0;
    return AnsiDeviceWidth(UI::Mapping(CurrentGrafPort->pix), text, byteCount);
}

/* The pen's v for DrawCString that puts the port font's capitals in the middle of the rows top..bottom - 1. */
short CapsCenteredPenV(short top, short bottom)
{
    TextStyle style;
    TextUnit capital = { 'H', 'H' };
    const RasterGlyph *glyph;
    int capHeight;

    if (CurrentGrafPort == NULL || !CurrentStyle(&style))
        return (short)((top + bottom) / 2);
    glyph = GlyphOf(&style.spec, &capital);
    capHeight = glyph != NULL ? glyph->top : style.ascent;
    return (short)((top + bottom + capHeight) / 2 + CurrentGrafPort->txSize - style.ascent);
}

/* TextWidth measured the way DrawTextRun draws: as Mac Roman (TextWidth measures the bytes as ANSI, so
   characters such as the ellipsis come out wrong) */
short TextWidthMacRoman(const char *text, int byteCount)
{
    TextUnit *units;
    short width;

    if (CurrentGrafPort == NULL || text == NULL || byteCount <= 0)
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    FromMacRoman(text, byteCount, units);
    UI::Mapping map(CurrentGrafPort->pix);
    width = ClampShort(map.LogicalLength(DeviceWidthOf(map, units, byteCount)));
    DeallocAPointer(units);
    return width;
}

/* the current font's line height: ascent + descent */
int TextLineHeight(void)
{
    TextStyle style;

    if (!CurrentStyle(&style))
        return 0;
    return style.ascent + style.descent;
}

/* the height of the text wrapped into `width' */
// FUNCTION: EVNOVA 0x004bcc70
int MeasureWrappedText(const char *text, int byteCount, int width)
{
    TextUnit *units;
    TextStyle style;
    FontSpec glyphs;
    int count;
    int lines;
    int start;
    int end;
    int next;

    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || byteCount == 0)
        return 0;
    UI::Mapping map(CurrentGrafPort->pix);
    if (!CurrentStyle(&style) || !FontAt(&glyphs, map.ScaleY()))
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    count = FromAnsi(text, byteCount, units);
    lines = 0;
    start = 0;
    while (NextLine(map, &glyphs, units, count, start, width, &end, &next)) {
        lines++;
        start = next;
    }
    DeallocAPointer(units);
    return lines * (style.ascent + style.descent);
}
