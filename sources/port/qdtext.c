// COMPILE: -lang c++ -bool off
/* qdtext.c -- the port's QuickDraw text calls, DrawTextRun .. MeasureWrappedText; exe 0x004bc760..0x004bcc70.
   The exe has GDI's DrawText draw, measure and wrap; the same is done here on port/fontraster.c, with
   DrawText's rules as far as the game meets them:
   - a line's first pixel row is the rectangle's top; the baseline lies the font's ascent below it; a wrapped
     line takes ascent + descent;
   - with word wrap a line ends at CR, LF or CR LF, or before the word that no longer fits, at the last
     space; a word wider than the line is not cut;
   - measured without DT_NOPREFIX an `&' is an accelerator mark and has no width (`&&' is one `&').
   Notes: Research/notes/toolbox.md */

#include <stddef.h>
#include <string.h>
#include "macport.h"
#include "gametypes.h"
#include "burger.h"
#include "player.h"
#include "cefix.h"
#include "fontraster.h"

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
    unsigned char macRoman;             /* its Mac Roman code when it came from Mac Roman text, else 0 */
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
        out[n].macRoman = 0;
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

/* The font, size and style set in the port. A font number the font cache does not know (the scrolling
   text asks for 20, the Mac's number for Times) leaves the port with the font it drew or measured with
   before, as GDI does when the exe selects no font; a port that has had none gets the system font. */
static int CurrentStyle(TextStyle *style)
{
    CGrafPtr port;

    port = CurrentGrafPort;
    if (GetFont(port->txFont, port->txSize, port->txFace, &style->spec)) {
        port->lastFont = port->txFont;
        port->lastSize = port->txSize;
        port->lastFace = port->txFace;
        port->hasLastFont = 1;
    } else if (!(port->hasLastFont && GetFont(port->lastFont, port->lastSize, port->lastFace, &style->spec))
               && !GetFont(kFallbackFont, port->txSize, port->txFace, &style->spec)) {
        return 0;
    }
    RasterMetrics(style->spec.font, style->spec.pixelSize, &style->ascent, &style->descent);
    return 1;
}

static const RasterGlyph *GlyphOf(const TextStyle *style, const TextUnit *unit)
{
    if (unit->unicode < kFirstVisible)
        return NULL;                    /* a control character has no shape and no width */
    return RasterGetGlyph(style->spec.font, style->spec.pixelSize, style->spec.flags, unit->unicode,
                          unit->macRoman);
}

static int WidthOf(const TextStyle *style, const TextUnit *units, int count)
{
    const RasterGlyph *glyph;
    int width;
    int i;

    width = 0;
    for (i = 0; i < count; i++) {
        glyph = GlyphOf(style, &units[i]);
        if (glyph != NULL)
            width += glyph->advance;
    }
    return width;
}

/* One line of word-wrapped text from `start' on: its characters end before `end', the next line begins at
   `next'. 0 = no line left. */
static int NextLine(const TextStyle *style, const TextUnit *units, int count, int start, int maxWidth,
                    int *end, int *next)
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
        glyph = GlyphOf(style, &units[i]);
        if (c == kSpace) {
            lastSpace = i;
        } else if (glyph != NULL && width + glyph->advance > maxWidth && lastSpace >= 0) {
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

/* the port's foreground colour through the gamma ramp, a channel in 8 bits, as the exe hands it to GDI */
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
                    r = ((old >> 10) & 31) + ((r - (int)((old >> 10) & 31)) * a + kFull / 2) / kFull;
                    g = ((old >> 5) & 31) + ((g - (int)((old >> 5) & 31)) * a + kFull / 2) / kFull;
                    b = (old & 31) + ((b - (int)(old & 31)) * a + kFull / 2) / kFull;
                }
                *(unsigned short *)p = (r << 10) | (g << 5) | b;
            } else {
                p[0] = p[0] + ((color->b - p[0]) * a + kFull / 2) / kFull;
                p[1] = p[1] + ((color->g - p[1]) * a + kFull / 2) / kFull;
                p[2] = p[2] + ((color->r - p[2]) * a + kFull / 2) / kFull;
            }
        }
    }
}

/* characters from x on, on one baseline; returns the x after them */
static int DrawUnits(PixMap *pix, const TextStyle *style, const TextUnit *units, int count, int x, int baseline,
                     const ClipRect *clip, const TextColor *color)
{
    const RasterGlyph *glyph;
    int i;

    for (i = 0; i < count; i++) {
        glyph = GlyphOf(style, &units[i]);
        if (glyph == NULL)
            continue;
        if (pix != NULL)
            DrawGlyph(pix, glyph, x, baseline, clip, color);
        x += glyph->advance;
    }
    return x;
}

/* Rectangles are in the pixel map's own pixels, the port's bounds not applied to `box', as the exe gives
   them to GDI. */
// FUNCTION: EVNOVA 0x004bc760
void DrawTextRun(const void *text, uint32_t byteCount, Rect *box, char clipped, int height)
{
    CGrafPtr port;
    PixMap *pix;
    TextUnit *units;
    TextStyle style;
    TextColor color;
    ClipRect clip;
    int x;
    int y;
    int start;
    int end;
    int next;

    if (text == NULL || byteCount == 0)
        return;
    port = CurrentGrafPort;
    if (port == NULL || !CurrentStyle(&style))
        return;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return;
    FromMacRoman((const char *)text, byteCount, units);

    /* the exe draws on the screen port through the window, where it does not reach the game's picture */
    pix = (port->offscreen != 0 && (port->pix.pixelSize == 16 || port->pix.pixelSize == 32)) ? &port->pix : NULL;
    color.r = (unsigned char)(gGammaRamp[port->foreColor.red >> 11] * 8);
    color.g = (unsigned char)(gGammaRamp[port->foreColor.green >> 11] * 8);
    color.b = (unsigned char)(gGammaRamp[port->foreColor.blue >> 11] * 8);

    clip.left = 0;
    clip.top = 0;
    clip.right = port->pix.width;
    clip.bottom = port->pix.height;
    x = port->pnLoc.h - port->pix.bounds.left;
    y = port->pnLoc.v - port->pix.bounds.top;
    if (clipped == 0) {
        y -= port->txSize;
        if (box != NULL) {
            if (clip.left < x)
                clip.left = x;
            if (clip.top < y)
                clip.top = y;
            if (clip.right > box->right)
                clip.right = box->right;
            if (clip.bottom > box->bottom)
                clip.bottom = box->bottom;
        }
        port->pnLoc.h += DrawUnits(pix, &style, units, byteCount, x, y + style.ascent, &clip, &color) - x;
    } else {
        if (clip.left < box->left)
            clip.left = box->left;
        if (clip.top < box->top)
            clip.top = box->top;
        if (clip.right > box->right)
            clip.right = box->right;
        if (clip.bottom > box->bottom)
            clip.bottom = box->bottom;
        y = box->top - height;
        start = 0;
        while (NextLine(&style, units, byteCount, start, box->right - box->left, &end, &next)) {
            DrawUnits(pix, &style, units + start, end - start, box->left, y + style.ascent, &clip, &color);
            y += style.ascent + style.descent;
            start = next;
        }
    }
    DeallocAPointer(units);
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

/* BUG (exe, all Windows versions): StringWidth, MeasureCString, TextWidth and MeasureWrappedText take the
   bytes as ANSI text with `&' as an accelerator mark, while DrawTextRun draws them as Mac Roman with `&'
   shown: text with characters above 0x7f or with an `&' is measured at another width than it is drawn. */
static short MeasureAnsi(const char *text, int byteCount)
{
    TextUnit *units;
    TextStyle style;
    short width;

    if (byteCount <= 0 || !CurrentStyle(&style))
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    width = WidthOf(&style, units, FromAnsi(text, byteCount, units));
    DeallocAPointer(units);
    return width;
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

/* TextWidth measured the way DrawTextRun draws: as Mac Roman (TextWidth measures the bytes as ANSI, so
   characters such as the ellipsis come out wrong) */
short TextWidthMacRoman(const char *text, int byteCount)
{
    TextUnit *units;
    TextStyle style;
    short width;

    if (CurrentGrafPort == NULL || text == NULL || byteCount <= 0 || !CurrentStyle(&style))
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    FromMacRoman(text, byteCount, units);
    width = WidthOf(&style, units, byteCount);
    DeallocAPointer(units);
    return width;
}

/* the height of the text wrapped into `width' */
// FUNCTION: EVNOVA 0x004bcc70
int MeasureWrappedText(const char *text, int byteCount, int width)
{
    TextUnit *units;
    TextStyle style;
    int count;
    int lines;
    int start;
    int end;
    int next;

    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || byteCount == 0)
        return 0;
    if (!CurrentStyle(&style))
        return 0;
    units = (TextUnit *)AllocAPointer(byteCount * sizeof(TextUnit));
    if (units == NULL)
        return 0;
    count = FromAnsi(text, byteCount, units);
    lines = 0;
    start = 0;
    while (NextLine(&style, units, count, start, width, &end, &next)) {
        lines++;
        start = next;
    }
    DeallocAPointer(units);
    return lines * (style.ascent + style.descent);
}
