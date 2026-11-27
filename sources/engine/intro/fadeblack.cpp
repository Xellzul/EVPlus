/* fadeblack.cpp -- fade out and paint the window black (win 0x004ab180), and the two startup splash screens that ride the same GWorldContainer/CopyBits plumbing as gworld.cpp. Notes: Research/notes/fadeblack.md */
#include "game.h"
#include "gameutil.h"
#include "render.h"
#include "stellar.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

#define kSplashPictID       0x83
#define kAmbrosiaLogoPictID 0x1fa4

Rect PaintLoadPagePicture(PicHandle pic)
{
    UI::Painter load(UI::Surface::Load());
    Rect box = loadPort.bounds;

    load.Fill(loadPort.bounds, UI::kBlack);
    if (pic != NULL) {
        CenterPict(pic, &box);
        load.Picture(pic, box);
    }
    return box;
}

void ShowLoadPage(const Rect &from, const Rect &to)
{
    UI::MenuScale menu;
    UI::Painter screen(UI::Surface::Screen());

    screen.Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
    screen.Copy(UI::Surface::Load(), from, to);
}

// FUNCTION: EVNOVA 0x004aaf60
// MAC: _ShowSplashScreen i386 0x00060e68 ppc 0x00068bfc
void ShowSplashScreen(void)
{
    UI::MenuScale menu;
    PicHandle pic;

    pic = GetPicture(kSplashPictID);
    PaintLoadPagePicture(pic);
    if (pic != NULL)
        QDReleaseResource(pic);
    ShowLoadPage(loadPort.bounds, *GetCWindowPortRect(gWind));
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
    box = PaintLoadPagePicture(pic);
    ShowLoadPage(box, box);
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
    BlackScreen();
}
