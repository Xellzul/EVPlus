/* mainscreenpict.cpp -- composite the main screen's background picture and its three animated button/logo sprites into MenuArt() each tick (win 0x0048c3c0). Notes: Research/notes/mainscreenpict.md */


#include "render.h"
#include "game.h"
#include "uiscale.h"
#include "painter.h"

// FUNCTION: EVNOVA 0x0048c3c0
// MAC: _MainScreenAnimDraw i386 0x00016146 ppc 0x00019eb4
void MainScreenAnimDraw(void)
{
    UI::MenuScale menu;
    Rect rect;
    short i;

    UI::Painter art{UI::Surface(*MenuArt())};

    if (logoPort.world != NULL) {
        rect.left = logoPort.bounds.left;
        rect.top = logoPort.bounds.top + gMainPicHeightDelta * logoIndex;
        rect.bottom = rect.top + gMainPicHeightDelta;
        rect.right = rect.left + gMainPicWidthDelta;

        logoDestRect.right = (rect.right - rect.left) + logoDestRect.left;
        logoDestRect.bottom = (rect.bottom - rect.top) + logoDestRect.top;

        art.Copy(UI::Surface(logoPort), rect, logoDestRect);
        ShowMenuArt(logoDestRect, true);
    }

    for (i = 0; i < 3; i++) {
        if (slidePort[i].world != NULL) {
            if (slideIndex[i] >= 0 && slideIndex[i] < slideFrameCount[i]) {
                rect.left = slidePort[i].bounds.left;
                rect.top = slidePort[i].bounds.top + slideFrameSize[i].height * slideIndex[i];
                rect.bottom = rect.top + slideFrameSize[i].height;
                rect.right = rect.left + slideFrameSize[i].width;

                slideDestRect[i].right = slideDestRect[i].left + (rect.right - rect.left);
                slideDestRect[i].bottom = slideDestRect[i].top + (rect.bottom - rect.top);

                art.Copy(UI::Surface(slidePort[i]), rect, slideDestRect[i]);
                ShowMenuArt(slideDestRect[i], true);
            }
        }
    }
}
