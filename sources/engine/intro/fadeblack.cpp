/* fadeblack.cpp -- fade out and paint the window black (win 0x004ab180), and the two startup splash screens that ride the same GWorldContainer/CopyBits plumbing as gworld.cpp. Notes: Research/notes/fadeblack.md */
#include "gameutil.h"
#include "render.h"
#include "stellar.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

#define kSplashPictID       0x83
#define kAmbrosiaLogoPictID 0x1fa4

static bool loadPageShown;           /* the load page shows a picture of the startup or the intro */
static short loadPagePictID;        /* which one; 0 = black alone */

Rect PaintLoadPagePicture(PicHandle pic, short id)
{
    UI::Painter load(UI::Surface::Load());
    Rect box = loadPort.bounds;

    load.Fill(loadPort.bounds, UI::kBlack);
    if (pic != NULL) {
        CenterPict(pic, &box);
        load.Picture(pic, box);
    }
    loadPageShown = true;
    loadPagePictID = pic != NULL ? id : 0;
    return box;
}

void ForgetLoadPagePicture(void)
{
    loadPageShown = false;
}

bool RedrawLoadPagePicture(void)
{
    UI::MenuScale menu;
    PicHandle pic;

    if (!loadPageShown)
        return false;
    pic = loadPagePictID != 0 ? GetPicture(loadPagePictID) : NULL;
    PaintLoadPagePicture(pic, loadPagePictID);
    if (pic != NULL)
        QDReleaseResource(pic);
    if (gameLoaded == 0 && progressMax > 0)
        PaintIntroProgressBar();
    return true;
}

// FUNCTION: EVNOVA 0x004aaf60
// MAC: _ShowSplashScreen i386 0x00060e68 ppc 0x00068bfc
void ShowSplashScreen(void)
{
    UI::MenuScale menu;
    PicHandle pic;
    Rect *rect;

    pic = GetPicture(kSplashPictID);
    rect = &loadPort.bounds;
    PaintLoadPagePicture(pic, kSplashPictID);
    if (pic != NULL)
        QDReleaseResource(pic);
    UI::Painter screen(UI::Surface::Screen());
    screen.Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
    screen.Copy(UI::Surface::Load(), *rect, *GetCWindowPortRect(gWind));
    Screen::Changed();
    Screen::EndFrame();
}


// FUNCTION: EVNOVA 0x004ab070
// MAC: _ShowAmbrosiaLogo i386 0x00060fb0 ppc 0x00068d54
void ShowAmbrosiaLogo(void)
{
    UI::MenuScale menu;
    PicHandle pic;
    Rect box;

    pic = GetPicture(kAmbrosiaLogoPictID);
    PaintLoadPagePicture(pic, kAmbrosiaLogoPictID);
    box = *GetCWindowPortRect((WindowRef)gWind);
    CenterPict(pic, &box);
    {
        UI::Painter screen(UI::Surface::Screen());

        screen.Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
        screen.Picture(pic, box);
    }
    if (pic != NULL)
        QDReleaseResource(pic);
    FadeScreenIn();
    Screen::EndFrame();                     /* shown while loading runs, which has no frame end of its own */
}

// FUNCTION: EVNOVA 0x004ab180
// MAC: _HideAmbrosiaLogo i386 0x0006108f ppc 0x00068e40
void HideAmbrosiaLogo(void)
{
    FadeScreenOut();
    UI::Painter(UI::Surface::Screen()).Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
}
