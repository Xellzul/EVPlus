/* fadeblack.cpp -- fade out and paint the window black (win 0x004ab180), and the two startup splash screens that ride the same GWorldContainer/CopyBits plumbing as gworld.cpp. Notes: Research/notes/fadeblack.md */
#include "gameutil.h"
#include "render.h"
#include "stellar.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

#define kSplashPictID       0x83
#define kAmbrosiaLogoPictID 0x1fa4

// FUNCTION: EVNOVA 0x004aaf60
// MAC: _ShowSplashScreen i386 0x00060e68 ppc 0x00068bfc
void ShowSplashScreen(void)
{
    UI::MenuScale menu;
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kSplashPictID);
    rect = &loadPort.bounds;
    {
        UI::Painter load(UI::Surface::Load());

        load.Fill(*rect, UI::kBlack);
        box = loadPort.bounds;
        if (pic != 0) {
            CenterPict(pic, &box);
            load.Picture((PicHandle)pic, box);
            QDReleaseResource((PicHandle)pic);
        }
    }
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
    void *pic;
    Rect box;

    pic = GetPicture(kAmbrosiaLogoPictID);
    box = loadPort.bounds;
    CenterPict(pic, &box);
    {
        UI::Painter load(UI::Surface::Load());

        load.Fill(loadPort.bounds, UI::kBlack);
        load.Picture((PicHandle)pic, box);
    }
    box = *GetCWindowPortRect((WindowRef)gWind);
    CenterPict(pic, &box);
    {
        UI::Painter screen(UI::Surface::Screen());

        screen.Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
        screen.Picture((PicHandle)pic, box);
    }
    if (pic != 0)
        QDReleaseResource((PicHandle)pic);
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
