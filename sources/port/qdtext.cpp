/* qdtext.cpp -- the port's QuickDraw text calls, DrawTextRun .. MeasureWrappedText; exe 0x004bc760..0x004bcc70.
   The exe has GDI's DrawText draw, measure and wrap; here the text is laid out by port/textlayout.cpp on
   port/fontraster.cpp's glyphs: this file gives it the current port's font, pen and pixels.
   Notes: Research/notes/toolbox.md */

#include "burger.h"
#include "fontraster.h"
#include "pixel.h"
#include "textlayout.h"

#define kFull           255
#define kFallbackFont   0               /* the system font's number */

using TextLayout::Unit;

/* -- the port's font ----------------------------------------------------------------------------------------- */

typedef struct TextStyle {
    FontSpec spec;
    int ascent;
    int descent;
} TextStyle;

/* The font, size and style set in the port, scaled by `scale': 1 gives the port's own size (rows: ascent and
   descent), the port's device scale the glyphs drawn and measured. A scaled font never uses a bitmap strike,
   so its glyphs are the same outlines at every scale. A font number the font cache does not know (the
   scrolling text asks for 20, the Mac's number for Times) leaves the port with the font it drew or measured
   with before; a port that has had none gets font 0 at the port's size. */
static int FontAt(FontSpec *spec, double scale)
{
    CGrafPtr port;

    port = CurrentGrafPort;
    if (GetFont(port->txFont, TextLayout::DeviceSize(port->txSize, scale), port->txFace, spec)) {
        port->lastFont = port->txFont;
        port->lastSize = port->txSize;
        port->lastFace = port->txFace;
        port->hasLastFont = 1;
    } else if (!(port->hasLastFont
                 && GetFont(port->lastFont, TextLayout::DeviceSize(port->lastSize, scale), port->lastFace, spec))
               && !GetFont(kFallbackFont, TextLayout::DeviceSize(port->txSize, scale), port->txFace, spec)) {
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

/* -- drawing ------------------------------------------------------------------------------------------------- */

/* `old' moved towards `target' by coverage `a' of kFull, to the nearest step either way */
static int Blend(int old, int target, int a)
{
    int moved;

    moved = (target - old) * a;
    return old + (moved >= 0 ? moved + kFull / 2 : moved - kFull / 2) / kFull;
}

/* a text colour, a channel in 8 bits, as the exe hands it to GDI */
typedef struct TextColor {
    int r;
    int g;
    int b;
} TextColor;

static void DrawGlyph(PixMap *pix, const RasterGlyph *glyph, int x, int baseline, const TextLayout::Clip *clip,
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

/* `rgb' as a TextColor, at 5 bits a channel */
static TextColor ColorOf(const RGBColor &rgb)
{
    TextColor color;

    color.r = (unsigned char)((rgb.red >> 11) * 8);
    color.g = (unsigned char)((rgb.green >> 11) * 8);
    color.b = (unsigned char)((rgb.blue >> 11) * 8);
    return color;
}

/* the layout's glyphs blended into a pixel map */
class PixelSink : public TextLayout::Sink {
public:
    PixelSink(PixMap *pix, const RGBColor &ink) : pix(pix), color(ColorOf(ink)) {}

    void Glyph(const RasterGlyph &glyph, int x, int baseline, const RGBColor *colour,
               const TextLayout::Clip &clip) override
    {
        if (colour == NULL) {
            DrawGlyph(pix, &glyph, x, baseline, &clip, &color);
            return;
        }
        TextColor span = ColorOf(*colour);
        DrawGlyph(pix, &glyph, x, baseline, &clip, &span);
    }

private:
    PixMap *pix;
    TextColor color;
};

/* the current port's font, surface and pen as the layout takes them; 0 when it has no port or no font */
int PortTextSetup(TextLayout::Setup *setup, TextLayout::Pen *pen)
{
    CGrafPtr port;
    TextStyle layout;

    port = CurrentGrafPort;
    if (port == NULL || !CurrentStyle(&layout))
        return 0;
    if (!FontAt(&setup->glyphs, UI::Mapping(port->pix).ScaleY()))
        return 0;
    setup->ascent = layout.ascent;
    setup->descent = layout.descent;
    setup->size = port->txSize;
    setup->width = port->pix.width;
    setup->height = port->pix.height;
    setup->left = port->pix.bounds.left;
    setup->top = port->pix.bounds.top;
    pen->h = port->pnLoc.h;
    pen->v = port->pnLoc.v;
    pen->deviceOffset = port->pnDeviceOffset;
    return 1;
}

/* Rectangles are in the pixel map's own pixels, the port's bounds not applied to `box', as the exe gives
   them to GDI. Rows and line pitch are in logical pixels at the port's size; widths and line breaks come from
   the device advances of the glyphs drawn. Unclipped text starts at the pen's device x and leaves the pen at
   the logical pixel nearest its end, the rest in pnDeviceOffset, so the next run continues exactly. clipped != 0
   wraps into `box' and needs one; then the height of the wrapped lines is returned. draw 0 only measures. */
static int DrawTextCore(const void *text, uint32_t byteCount, Rect *box, char clipped, int height, char draw)
{
    CGrafPtr port;
    TextLayout::Setup setup;
    TextLayout::Pen pen;
    int wrapped;

    if (text == NULL || byteCount == 0 || (clipped != 0 && box == NULL))
        return 0;
    if (!PortTextSetup(&setup, &pen))
        return 0;
    port = CurrentGrafPort;

    /* the exe draws on the screen port through the window, where it does not reach the game's picture */
    PixelSink pixels(&port->pix, port->foreColor);
    bool drawn = draw != 0 && port->offscreen != 0 && (port->pix.pixelSize == 16 || port->pix.pixelSize == 32);
    wrapped = TextLayout::Draw(drawn ? &pixels : NULL, UI::Mapping(port->pix), setup, text, byteCount, box,
                               clipped != 0, height, NULL, 0, pen);
    port->pnLoc.h = pen.h;
    port->pnDeviceOffset = pen.deviceOffset;
    return wrapped;
}

// FUNCTION: EVNOVA 0x004bc760
void DrawTextRun(const void *text, uint32_t byteCount, Rect *box, char clipped, int height)
{
    DrawTextCore(text, byteCount, box, clipped, height, 1);
}

/* the height of the text wrapped into `box' as the Painter's TextSpans wraps it */
int MeasureTextSpans(const void *text, uint32_t byteCount, Rect *box)
{
    return DrawTextCore(text, byteCount, box, 1, 0, 0);
}

// FUNCTION: EVNOVA 0x004bcab0
void DrawCString(const char *text)
{
    DrawTextRun(text, strlen(text), NULL, 0, 0);
}

/* -- measuring ----------------------------------------------------------------------------------------------- */

/* the device advances of the glyphs DrawTextRun draws for these characters on the current port */
static int DeviceWidthOf(const UI::Mapping &map, const Unit *units, int count)
{
    FontSpec glyphs;

    if (!FontAt(&glyphs, map.ScaleY()))
        return 0;
    return TextLayout::Width(glyphs, units, count);
}

/* BUG (exe, all Windows versions): MeasureCString and MeasureWrappedText take the bytes as ANSI text
   with `&' as an accelerator mark, while DrawTextRun draws them as Mac Roman with `&' shown: text with
   characters above 0x7f or with an `&' is measured at another width than it is drawn. */
static int AnsiDeviceWidth(const UI::Mapping &map, const char *text, int byteCount)
{
    Unit *units;
    int width;

    if (byteCount <= 0)
        return 0;
    units = (Unit *)AllocAPointer(byteCount * sizeof(Unit));
    if (units == NULL)
        return 0;
    width = DeviceWidthOf(map, units, TextLayout::FromAnsi(text, byteCount, units));
    DeallocAPointer(units);
    return width;
}

static short MeasureAnsi(const char *text, int byteCount)
{
    UI::Mapping map(CurrentGrafPort->pix);

    return TextLayout::ClampShort(map.LogicalLength(AnsiDeviceWidth(map, text, byteCount)));
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

/* The text's width in the current port's device pixels, before rounding to logical ones: where the glyphs of the
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
    Unit capital = { 'H', 'H' };
    const RasterGlyph *glyph;
    int capHeight;

    if (CurrentGrafPort == NULL || !CurrentStyle(&style))
        return (short)((top + bottom) / 2);
    glyph = TextLayout::GlyphOf(style.spec, capital);
    capHeight = glyph != NULL ? glyph->top : style.ascent;
    return (short)((top + bottom + capHeight) / 2 + CurrentGrafPort->txSize - style.ascent);
}

/* The text's width measured the way DrawTextRun draws: as Mac Roman (MeasureCString measures the bytes as ANSI,
   so characters such as the ellipsis come out wrong) */
short TextWidthMacRoman(const char *text, int byteCount)
{
    Unit *units;
    short width;

    if (CurrentGrafPort == NULL || text == NULL || byteCount <= 0)
        return 0;
    units = (Unit *)AllocAPointer(byteCount * sizeof(Unit));
    if (units == NULL)
        return 0;
    TextLayout::FromMacRoman(text, byteCount, units);
    UI::Mapping map(CurrentGrafPort->pix);
    width = TextLayout::ClampShort(map.LogicalLength(DeviceWidthOf(map, units, byteCount)));
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
    Unit *units;
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
    units = (Unit *)AllocAPointer(byteCount * sizeof(Unit));
    if (units == NULL)
        return 0;
    count = TextLayout::FromAnsi(text, byteCount, units);
    lines = 0;
    start = 0;
    while (TextLayout::NextLine(map, glyphs, units, count, start, width, &end, &next)) {
        lines++;
        start = next;
    }
    DeallocAPointer(units);
    return lines * (style.ascent + style.descent);
}
