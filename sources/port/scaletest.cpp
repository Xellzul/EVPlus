/* scaletest.cpp -- a test page for the drawing code: every kind of drawing the UI uses (rects, lines, ovals,
   text, pictures, icons, patterns, copy modes, masks) on one fixed 640 x 480 page, drawn at UI scale 1 into
   scaletest.raw and at the other test scales into scaletest_<scale>.raw, shown before the main screen when
   kScaleTest (mainscreen.cpp) is 1. The text checks are logged to scaletest.txt. Scale 1 must give the same
   file before and after a change; build/devtools/scaletest.sh runs it. */

#include <stdio.h>
#include <string.h>
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#include "game.h"
#include "render.h"
#include "platform.h"
#include "uiscale.h"
#include "scaleddraw.h"

namespace {

constexpr short kPageWidth = 640;
constexpr short kPageHeight = 480;
constexpr double kTestScales[] = { 1.25, 1.875, 2.0, 1.0 };     /* 1 last: that page is shown */

constexpr short kPictBig = 133;             /* "new key background", a large picture */
constexpr short kPictArrow = 134;           /* "Up arrow", a tiny one */
constexpr short kIconMapArrow = 15000;      /* a cicn: drawn with srcOr */
constexpr short kPatternStatic = 128;       /* the radar static ppat */

constexpr short kModeAddOver = 0x22;
constexpr short kModeAdMax = 0x25;

void Color(unsigned short red, unsigned short green, unsigned short blue)
{
    RGBColor color = { red, green, blue };

    RGBForeColor(&color);
}

/* filled, erased, inverted and framed rects, and rects side by side that must not leave a seam */
void DrawRects()
{
    Rect r;

    Color(0xffff, 0, 0);
    SetRect(&r, 10, 10, 60, 40);
    PaintRect(&r);
    Color(0, 0xffff, 0);
    SetRect(&r, 60, 10, 110, 40);
    PaintRect(&r);
    Color(0, 0, 0xffff);
    SetRect(&r, 110, 10, 160, 40);
    PaintRect(&r);

    SetRect(&r, 20, 15, 150, 35);
    InvertRect(&r);

    Color(0xffff, 0xffff, 0xffff);
    SetRect(&r, 170, 10, 230, 40);
    FrameRect(&r);
    SetRect(&r, 175, 15, 225, 35);
    EraseRect(&r);
}

/* thin and thick lines in every direction */
void DrawLines()
{
    Color(0xffff, 0xffff, 0);
    PenSize(1, 1);
    MoveTo(250, 10);
    LineTo(330, 40);
    MoveTo(250, 40);
    LineTo(330, 10);
    PenSize(3, 3);
    MoveTo(340, 10);
    LineTo(340, 40);
    MoveTo(350, 25);
    LineTo(420, 25);
    PenSize(1, 1);
}

void DrawOvals()
{
    Rect r;

    Color(0, 0xffff, 0xffff);
    SetRect(&r, 430, 10, 460, 40);
    FrameOval(&r);
    SetRect(&r, 470, 10, 500, 40);
    PaintOval(&r);
}

/* single lines, a label followed by a value, centred text, and a wrapped paragraph in a box */
void DrawText()
{
    static const char kParagraph[] =
        "Word wrap: the Federation patrol hails you as you leave the system, asking whether you have seen any "
        "pirate activity near the asteroid belt.";
    const char *centred = "Centred over the line";
    Rect box;
    short width;

    TextFont(smallTextFont);
    TextFace(0);
    Color(0xffff, 0xffff, 0xffff);
    short y = 50;
    for (short size = 9; size <= 18; size += 3) {
        y += size + 4;
        TextSize(size);
        MoveTo(10, y);
        DrawCString("The quick brown fox jumps over the lazy dog 0123456789");
    }

    TextSize(12);
    MoveTo(10, 142);
    DrawCString("Credits: ");
    Color(0xffff, 0xc000, 0);
    DrawCString("25,000");
    Color(0xffff, 0xffff, 0xffff);
    TextFace(kFontBold);
    DrawCString("  bold");
    TextFace(0);

    width = MeasureCString(centred);
    MoveTo(320 + (200 - width) / 2, 150);
    DrawCString(centred);
    MoveTo(320, 152);
    LineTo(520, 152);

    RGBColor background = { 0x2000, 0x2000, 0x4000 };
    SetRect(&box, 10, 160, 250, 230);
    RGBBackColor(&background);
    TETextBox(kParagraph, strlen(kParagraph), &box, 0);
    BackColor(blackColor);
    InsetRect(&box, -1, -1);
    Color(0x8000, 0x8000, 0x8000);
    FrameRect(&box);
}

/* a device pixel of the page, 16 bits */
unsigned short PagePixel(const PixMap *pix, int x, int y)
{
    return *(const unsigned short *)((const char *)pix->baseAddr + (size_t)y * pix->rowBytes + (size_t)x * 2);
}

/* A green square when `ok', else a red one. */
void Mark(short left, short top, bool ok)
{
    Rect r;

    Color(ok ? 0 : 0xffff, ok ? 0xc000 : 0, 0);
    SetRect(&r, left, top, left + 6, top + 6);
    PaintRect(&r);
}

/* Growing prefixes of one string, one per row at the same x, as typed into a field. Left square: the
   pixels left of each prefix's end are the same in the next row. Right square: StringWidth == the pen's
   advance. Each row is also logged. */
void DrawPrefixRows(GWorldContainer *page, short left, short size, double scale, FILE *log)
{
    static const char kTyped[] = "Kestrel Wing";
    constexpr int kRows = sizeof(kTyped) - 1;
    PixMapPtr pix = (PixMapPtr)GWorldContainerBits(page);
    UI::Mapping map(*pix);
    unsigned char pascal[256];
    short widths[kRows];
    constexpr short kTop = 280;
    constexpr short kPitch = 16;            /* whole device pixels at every test scale */

    TextFont(smallTextFont);
    TextFace(0);
    TextSize(size);
    for (int k = 1; k <= kRows; k++) {
        short pen = (short)(kTop + (k - 1) * kPitch + size);

        Color(0xffff, 0xffff, 0xffff);
        MoveTo(left, pen);
        ::DrawText(kTyped, 0, k);
        int advance = CurrentGrafPort->pnLoc.h - left;
        pascal[0] = (unsigned char)k;
        memcpy(pascal + 1, kTyped, k);
        widths[k - 1] = StringWidth(pascal);
        Mark((short)(left - 8), (short)(pen - 6), widths[k - 1] == advance);
        if (log != NULL)
            fprintf(log, "scale %g size %d prefix %2d: StringWidth %d pen advance %d%s\n", scale, size, k,
                    widths[k - 1], advance, widths[k - 1] == advance ? "" : "  MISMATCH");
    }
    for (int k = 1; k < kRows; k++) {
        int rowTop = kTop + (k - 1) * kPitch;
        int y0 = map.Y(rowTop);
        int y1 = map.Y(rowTop + kPitch);
        int x0 = map.X(left);
        int x1 = map.X(left + widths[k - 1] - 1);
        bool same = true;

        for (int y = y0; y < y1 && same; y++) {
            for (int x = x0; x < x1; x++) {
                if (PagePixel(pix, x, y) != PagePixel(pix, x, y + y1 - y0)) {
                    same = false;
                    break;
                }
            }
        }
        Mark((short)(left - 16), (short)(rowTop + size - 6), same);
        if (log != NULL && !same)
            fprintf(log, "scale %g size %d prefix %2d: pixels differ in the next row\n", scale, size, k);
    }
}

/* the same device rows of two bands `shift' rows apart are the same from column x0 to x1 */
bool SameBands(const PixMap *pix, int x0, int x1, int y0, int y1, int shift)
{
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (PagePixel(pix, x, y) != PagePixel(pix, x, y + shift))
                return false;
        }
    }
    return true;
}

/* One string drawn whole, and below it in two DrawCString calls that go on from the pen: the pixels must be
   the same. */
void DrawPieceRows(GWorldContainer *page, short top, short size, double scale, FILE *log)
{
    static const char kWhole[] = "Credits: 25,000";
    constexpr short kLeft = 15;
    constexpr short kPitch = 24;            /* whole device pixels at every test scale */
    PixMapPtr pix = (PixMapPtr)GWorldContainerBits(page);
    UI::Mapping map(*pix);

    TextFont(smallTextFont);
    TextFace(0);
    TextSize(size);
    Color(0xffff, 0xffff, 0xffff);
    MoveTo(kLeft, top + size);
    DrawCString(kWhole);
    MoveTo(kLeft, top + kPitch + size);
    DrawCString("Credits: ");
    DrawCString("25,000");
    bool same = SameBands(pix, map.X(kLeft), map.X(135), map.Y(top), map.Y(top + 20), map.Y(top + kPitch) - map.Y(top));
    Mark(kLeft - 11, (short)(top + kPitch + size - 6), same);
    if (log != NULL)
        fprintf(log, "scale %g size %d pieces: %s\n", scale, size, same ? "same as whole" : "DIFFER from whole");
}

/* A string, and where TextDeviceWidth puts its end (the edit field's caret): that is the pen's device x after
   drawing it, and the glyphs' ink ends at most two logical pixels before it, never after. The caret is drawn
   there afterwards, one logical pixel wide, left of the end. */
void DrawCaretRow(GWorldContainer *page, short left, short top, short size, const char *text, double scale,
                  FILE *log)
{
    PixMapPtr pix = (PixMapPtr)GWorldContainerBits(page);
    UI::Mapping map(*pix);
    int count = (int)strlen(text);
    RGBColor caretColor = { 0xffff, 0x8000, 0 };
    Rect caret;

    TextFont(smallTextFont);
    TextFace(0);
    TextSize(size);
    Color(0xffff, 0xffff, 0xffff);
    MoveTo(left, top + size);
    ::DrawText(text, 0, count);
    int end = map.X(left) + TextDeviceWidth(text, count);
    int penEnd = map.X(CurrentGrafPort->pnLoc.h) + CurrentGrafPort->pnDeviceOffset;
    int ink = map.X(left);
    for (int y = map.Y(top); y < map.Y(top + 18) && y < pix->height; y++) {
        for (int x = map.X(left); x < map.X(left + 120); x++) {
            if (PagePixel(pix, x, y) != 0 && x + 1 > ink)
                ink = x + 1;
        }
    }
    int gap = end - ink;
    bool ok = end == penEnd && gap >= 0 && gap <= map.X(left + 2) - map.X(left);
    SetRect(&caret, (short)(end - (map.X(left + 1) - map.X(left))), (short)map.Y(top + 3), (short)end,
            (short)map.Y(top + size + 2));
    Scaled::FillDeviceRect(caret, caretColor);
    Mark((short)(left - 10), (short)(top + size - 6), ok);
    if (log != NULL)
        fprintf(log, "scale %g size %d caret \"%s\": end %d pen %d ink %d gap %d%s\n", scale, size, text,
                end - map.X(left), penEnd - map.X(left), ink - map.X(left), gap, ok ? "" : "  FAIL");
}

/* pictures at their size and stretched, a cicn, a tiled pattern */
void DrawPictures()
{
    PicHandle big = GetPicture(kPictBig);
    PicHandle arrow = GetPicture(kPictArrow);
    Image_t *icon = GetCIcon(kIconMapArrow);
    Image_t *pattern = GetPixPat(kPatternStatic);
    Rect r;

    if (big != NULL) {
        QDGetPictureBounds(big, &r);
        OffsetRect(&r, 270 - r.left, 170 - r.top);
        r.right = r.left + 160;
        r.bottom = r.top + 100;
        DrawPicHandle(big, &r);
        QDReleaseResource(big);
    }
    if (arrow != NULL) {
        SetRect(&r, 450, 170, 461, 179);
        DrawPicHandle(arrow, &r);
        SetRect(&r, 470, 170, 503, 197);
        DrawPicHandle(arrow, &r);
        QDReleaseResource(arrow);
    }
    if (icon != NULL) {
        QDGetCIconBounds(icon, &r);
        OffsetRect(&r, 520 - r.left, 170 - r.top);
        nv_BlitImageOr(&r, icon);
    }
    if (pattern != NULL) {
        SetRect(&r, 450, 210, 630, 260);
        FillCRect(&r, pattern);
    }
}

/* the two blend modes and a masked copy, from a second page with a gradient */
void DrawCopyModes(GWorldContainer *page)
{
    GWorldContainer source;
    GWorldContainer mask;
    Rect band;
    Rect area;
    Rect r;

    SetRect(&area, 0, 0, 120, 60);
    source.world = 0;
    mask.world = 0;
    CreateGWorldContainer(&source, GetMainDevice(), 0, &area);
    CreateGWorldContainer(&mask, GetMainDevice(), 0, &area);
    if (source.world == 0 || mask.world == 0)
        return;

    SetGWorldContainer(&source);
    for (short x = 0; x < 120; x += 10) {
        Color(x * 500, 0x4000, 0xffff - x * 500);
        SetRect(&band, x, 0, x + 10, 60);
        PaintRect(&band);
    }
    SetGWorldContainer(&mask);
    Color(0xffff, 0xffff, 0xffff);
    PaintRect(&area);
    Color(0, 0, 0);
    SetRect(&band, 0, 0, 60, 60);
    PaintRect(&band);

    SetGWorldContainer(page);
    Color(0x6000, 0x6000, 0x6000);
    SetRect(&r, 140, 270, 390, 290);
    PaintRect(&r);
    SetRect(&r, 10, 250, 130, 310);
    CopyBits(GWorldContainerBits(&source), GWorldContainerBits(page), &area, &r, 0, NULL);
    SetRect(&r, 140, 250, 260, 310);
    CopyBits(GWorldContainerBits(&source), GWorldContainerBits(page), &area, &r, kModeAddOver, NULL);
    SetRect(&r, 270, 250, 390, 310);
    CopyBits(GWorldContainerBits(&source), GWorldContainerBits(page), &area, &r, kModeAdMax, NULL);
    SetRect(&r, 10, 320, 130, 380);
    CopyMask(GWorldContainerBits(&source), GWorldContainerBits(&mask), GWorldContainerBits(page), &area, &area, &r);
    SetRect(&r, 140, 320, 380, 440);
    CopyBits(GWorldContainerBits(&source), GWorldContainerBits(page), &area, &r, 0, NULL);

    DisposeGWorldContainer(&source);
    DisposeGWorldContainer(&mask);
}

/* the page's device pixels as raw2png.py reads them: width, height, then 16-bit pixels row by row */
void WritePage(GWorldContainer *page, double scale)
{
    PixMapPtr pix = (PixMapPtr)GWorldContainerBits(page);
    char name[64];
    int32_t size[2] = { pix->width, pix->height };

    if (scale == 1.0)
        snprintf(name, sizeof(name), "scaletest.raw");
    else
        snprintf(name, sizeof(name), "scaletest_%g.raw", scale);
    FILE *file = PlatFileOpen(name, "wb");
    if (file == NULL)
        return;
    fwrite(size, sizeof(size), 1, file);
    for (int y = 0; y < pix->height; y++)
        fwrite((const char *)pix->baseAddr + (size_t)y * pix->rowBytes, 2, pix->width, file);
    fclose(file);
}

void DrawPage(GWorldContainer *page, double scale, FILE *log)
{
    SetGWorldContainer(page);
    ForeColor(blackColor);
    PaintRect(&page->bounds);
    DrawRects();
    DrawLines();
    DrawOvals();
    DrawText();
    DrawPictures();
    DrawCopyModes(page);
    DrawPrefixRows(page, 416, 12, scale, log);
    DrawPrefixRows(page, 530, 16, scale, log);
    DrawPieceRows(page, 386, 12, scale, log);
    DrawPieceRows(page, 434, 16, scale, log);
    DrawCaretRow(page, 160, 444, 16, "Haxon", scale, log);
    DrawCaretRow(page, 280, 444, 16, "Lieutenant", scale, log);
    DrawCaretRow(page, 160, 464, 12, "Kestrel Wing", scale, log);
    DrawCaretRow(page, 280, 464, 12, "Credits: 25,000", scale, log);
}

} // namespace

void DebugScaleTest(void)
{
    GWorldContainer page;
    Rect bounds;

    FILE *log = PlatFileOpen("scaletest.txt", "wb");

    SetRect(&bounds, 0, 0, kPageWidth, kPageHeight);
    for (double scale : kTestScales) {
        page.world = 0;
        CreateGWorldContainer(&page, GetMainDevice(), 0, &bounds);
        if (page.world == 0)
            break;
        UI::ScalePortPixels(page.world, scale);
        DrawPage(&page, scale, log);
        WritePage(&page, scale);
        if (scale != 1.0)
            DisposeGWorldContainer(&page);
    }
    if (log != NULL)
        fclose(log);
    if (page.world == 0)
        return;

    SWSetPortToWindow(gSpriteWorld);
    CopyBits(GWorldContainerBits(&page), GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &bounds, &bounds, 0, NULL);
    QDFlushPortBuffer(GetWindowPort(gWind), 0);
    DisposeGWorldContainer(&page);
}
