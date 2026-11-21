/* qdshims.cpp -- small QuickDraw and Event Manager calls: page flip, mouse and buttons, text and colour state */

#include "render.h"
#include "uiscale.h"

/* ScreenEndScene (mac @ScreenEndScene@0). */
// FUNCTION: EVNOVA 0x004b6850
void ScreenEndScene(void)
{
    if (gDisableUpdatesMode == 0) {
        UpdateAndPageFlip();
        LockVideoPage();
        if (gGamePort.pix.baseAddr != videoBase) {
            gGamePort.pix.baseAddr = videoBase;
            if (gSpriteWorld != NULL)
                SWWindowMoved(gSpriteWorld);
        }
    }
}

// FUNCTION: EVNOVA 0x004b68a0
void GetGlobalMouse(Point *pt)
{
    int32_t h;
    int32_t v;

    MouseReadPosition(&h, &v);
    UI::Mapping screen(gGamePort.pix);
    pt->h = (short)screen.LogicalX(h);
    pt->v = (short)screen.LogicalY(v);
}

// FUNCTION: EVNOVA 0x004b68d0
void FlushEvents(unsigned short whichMask, unsigned short stopMask)
{
    KeyboardFlush();
}

// FUNCTION: EVNOVA 0x004b68e0
Boolean Button(void)
{
    KeyboardKbhit();
    return MouseReadButtons();
}

// FUNCTION: EVNOVA 0x004b68f0
Boolean StillDown(void)
{
    KeyboardKbhit();
    return MouseReadButtons();
}

// FUNCTION: EVNOVA 0x004b6900
void TextFont(short font)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txFont = font;
}

// FUNCTION: EVNOVA 0x004b6920
void TextSize(short size)
{
    // BUG (CE F12.82): CE's replacement drops the NULL-port test; no caller in play has no port
    CurrentGrafPort->txSize = size;
}

// FUNCTION: EVNOVA 0x004b6940
void TextFace(short face)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txFace = face;
}

// FUNCTION: EVNOVA 0x004b6960
uint32_t Color2Index(RGBColor *color)
{
    return ((color->red & 0xf800) >> 1) | ((color->green & 0xf800) >> 6) |
           ((color->blue & 0xf800) >> 11);
}

// FUNCTION: EVNOVA 0x004b6990
void RGBForeColor(const RGBColor *color)
{
    RGBColor *dst;

    if (CurrentGrafPort == NULL || color == NULL)
        return;
    dst = &CurrentGrafPort->foreColor;
    dst->red = color->red;
    dst->green = color->green;
    dst->blue = color->blue;
}

// FUNCTION: EVNOVA 0x004b69d0
void BackColor(const RGBColor *color)
{
    RGBColor *dst;

    if (CurrentGrafPort == NULL || color == NULL)
        return;
    dst = &CurrentGrafPort->backColor;
    dst->red = color->red;
    dst->green = color->green;
    dst->blue = color->blue;
}
