/* mainscreenpict.cpp -- composite the main screen's background picture and its three animated button/logo sprites into MenuArt() each tick (win 0x0048c3c0). Notes: Research/notes/mainscreenpict.md */


#include "dialogs.h"
#include "render.h"
#include "game.h"
#include "uiscale.h"
#include "painter.h"

static short shownLogo = -1;            /* the frames drawn into the menu's art since its background; -1 = none */
static short shownSlide[3] = { -1, -1, -1 };

/* the logo and the slides at these frames (-1: not drawn) into the menu's art */
static void DrawFrames(short logo, const short slide[3])
{
    UI::MenuScale menu;
    Rect rect;
    short i;

    UI::Painter art{UI::Surface(*MenuArt())};

    if (logoPort.world != NULL && logo >= 0) {
        rect.left = logoPort.bounds.left;
        rect.top = logoPort.bounds.top + gMainPicHeightDelta * logo;
        rect.bottom = rect.top + gMainPicHeightDelta;
        rect.right = rect.left + gMainPicWidthDelta;

        logoDestRect.right = (rect.right - rect.left) + logoDestRect.left;
        logoDestRect.bottom = (rect.bottom - rect.top) + logoDestRect.top;

        art.Copy(UI::Surface(logoPort), rect, logoDestRect);
        ShowMenuArt(logoDestRect, true);
        shownLogo = logo;
    }

    for (i = 0; i < 3; i++) {
        if (slidePort[i].world != NULL) {
            if (slide[i] >= 0 && slide[i] < slideFrameCount[i]) {
                rect.left = slidePort[i].bounds.left;
                rect.top = slidePort[i].bounds.top + slideFrameSize[i].height * slide[i];
                rect.bottom = rect.top + slideFrameSize[i].height;
                rect.right = rect.left + slideFrameSize[i].width;

                slideDestRect[i].right = slideDestRect[i].left + (rect.right - rect.left);
                slideDestRect[i].bottom = slideDestRect[i].top + (rect.bottom - rect.top);

                art.Copy(UI::Surface(slidePort[i]), rect, slideDestRect[i]);
                ShowMenuArt(slideDestRect[i], true);
                shownSlide[i] = slide[i];
            }
        }
    }
}

// FUNCTION: EVNOVA 0x0048c3c0
// MAC: _MainScreenAnimDraw i386 0x00016146 ppc 0x00019eb4
void MainScreenAnimDraw(void)
{
    DrawFrames(logoIndex, slideIndex);
}

void ForgetMainScreenAnim(void)
{
    shownLogo = -1;
    for (short &each : shownSlide)
        each = -1;
}

void RedrawMainScreenArt(void)
{
    short logo = shownLogo;
    short slide[3] = { shownSlide[0], shownSlide[1], shownSlide[2] };

    SetupInterfaceBackground();
    DrawFrames(logo, slide);
    mainScreenBackgroundDirty = 0;
}
