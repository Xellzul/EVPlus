/* scaletest.cpp -- a test page for the drawing code: every kind of drawing the UI uses (rects, lines, ovals,
   text, pictures, icons, patterns, copy modes, masks) on one fixed 640 x 480 page, drawn at UI scale 1 into
   scaletest.raw and at the other test scales into scaletest_<scale>.raw, shown before the main screen when
   kScaleTest (mainscreen.cpp) is 1. Scale 1 must give the same file before and after a change;
   build/devtools/scaletest.sh runs it. */

#include <stdio.h>
#include <string.h>
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#include "game.h"
#include "render.h"
#include "platform.h"
#include "uiscale.h"

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

void DrawPage(GWorldContainer *page)
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
}

} // namespace

void DebugScaleTest(void)
{
    GWorldContainer page;
    Rect bounds;

    SetRect(&bounds, 0, 0, kPageWidth, kPageHeight);
    for (double scale : kTestScales) {
        page.world = 0;
        CreateGWorldContainer(&page, GetMainDevice(), 0, &bounds);
        if (page.world == 0)
            return;
        UI::ScalePortPixels(page.world, scale);
        DrawPage(&page);
        WritePage(&page, scale);
        if (scale != 1.0)
            DisposeGWorldContainer(&page);
    }

    SWSetPortToWindow(gSpriteWorld);
    CopyBits(GWorldContainerBits(&page), GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &bounds, &bounds, 0, NULL);
    QDFlushPortBuffer(GetWindowPort(gWind), 0);
    DisposeGWorldContainer(&page);
}
