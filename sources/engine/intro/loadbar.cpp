/* loadbar.cpp -- the startup progress bar (win 0x004ab1b0..0x004ab3d0). Notes: Research/notes/loadbar.md */
#include "game.h"
#include "render.h"
#include "sound.h"
#include "uiscale.h"

/* the bar's rect comes out of the colr resource */

// FUNCTION: EVNOVA 0x004ab1b0
// MAC: _InitIntroProgressBar i386 0x00061263 ppc 0x0006904c
void InitIntroProgressBar(void)
{
    UI::MenuScale menu;
    Rect line;
    Rect fill;
    short midH;
    short midV;
    RGBColor *barColor;

    progressMax = CountResources(kShipType);
    progressCount = 0;

    midH = ((GetCWindowPortRect((WindowRef)gWind)->left
                    + GetCWindowPortRect((WindowRef)gWind)->right) / 2);
    midV = ((GetCWindowPortRect((WindowRef)gWind)->top
                    + GetCWindowPortRect((WindowRef)gWind)->bottom) / 2);

    progressRect.top = progressDefRect.top;
    progressRect.left = progressDefRect.left;
    progressRect.bottom = progressDefRect.bottom;
    progressRect.right = progressDefRect.right;
    OffsetRect(&progressRect, midH, midV);

    line.top = progressRect.top;
    line.left = progressRect.left;
    line.bottom = progressRect.bottom;
    line.right = progressRect.right;
    InsetRect(&line, 0, (line.bottom - line.top) / 2);

    if (line.bottom <= progressRect.bottom) {
        barColor = &progressColor[2];
        do {
        RGBForeColor(barColor);
        FrameRect(&line);
        ForeColor(blackColor);
        fill.top = line.top;
        fill.left = line.left;
        fill.bottom = line.bottom;
        fill.right = line.right;
        InsetRect(&fill, 1, 1);
        PaintRect(&fill);
        InsetRect(&line, 0, -1);
        HandleIntroMusic(0);
        Delay(2, &tix);
        } while (line.bottom <= progressRect.bottom);
    }
    DrawIntroProgressBar();
}

// FUNCTION: EVNOVA 0x004ab3b0
// MAC: _DeltaIntroProgressBar i386 0x000613f4 ppc 0x00069204
void DeltaIntroProgressBar(double amount)
{
    progressCount = progressCount + amount;
    DrawIntroProgressBar();
}

// FUNCTION: EVNOVA 0x004ab3d0
// MAC: _DrawIntroProgressBar i386 0x000610ba ppc 0x00068e7c
void DrawIntroProgressBar(void)
{
    UI::MenuScale menu;
    Rect box;
    box.left = gLoadBarBase.left;
    box.top = gLoadBarBase.top;
    box.bottom = gLoadBarBase.bottom;
    box.right = gLoadBarBase.right;
    box.right = box.left + 30;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    RGBForeColor(&progressColor[2]);
    FrameRect(&progressRect);

    box = progressRect;
    InsetRect(&box, 1, 1);

    box.right = (box.left + progressCount / progressMax * 198.0);
    if (box.right >= progressRect.right)
        box.right = progressRect.right - 1;

    RGBForeColor(&progressColor[0]);
    InsetRect(&box, 1, 1);
    PaintRect(&box);
    RGBForeColor(&progressColor[1]);
    InsetRect(&box, -1, -1);
    FrameRect(&box);
    ForeColor(blackColor);
    box.left = box.right;
    box.right = progressRect.right - 1;
    PaintRect(&box);

    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &progressRect, &progressRect, 0, 0);
    ScreenEndScene();
}
