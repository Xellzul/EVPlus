/* loadbar.cpp -- the startup progress bar (win 0x004ab1b0..0x004ab3d0). Notes: Research/notes/loadbar.md */
#include "game.h"
#include "render.h"
#include "sound.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

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
        fill.top = line.top;
        fill.left = line.left;
        fill.bottom = line.bottom;
        fill.right = line.right;
        InsetRect(&fill, 1, 1);
        {
            UI::Painter screen(UI::Surface::Screen());
            screen.Frame(line, *barColor);
            screen.Fill(fill, UI::kBlack);
        }
        Screen::Changed();                  /* each step shown by the Delay's wait */
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

    {
        UI::Painter load(UI::Surface::Load());

        load.Frame(progressRect, progressColor[2]);

        box = progressRect;
        InsetRect(&box, 1, 1);

        box.right = (box.left + progressCount / progressMax * 198.0);
        if (box.right >= progressRect.right)
            box.right = progressRect.right - 1;

        InsetRect(&box, 1, 1);
        load.Fill(box, progressColor[0]);
        InsetRect(&box, -1, -1);
        load.Frame(box, progressColor[1]);
        box.left = box.right;
        box.right = progressRect.right - 1;
        load.Fill(box, UI::kBlack);
    }

    UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), progressRect, progressRect);
    Screen::Changed();
    Screen::EndFrame();                     /* loading has no other frame end */
}
