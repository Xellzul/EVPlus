// COMPILE: -lang c++ -bool off
/* fadeblack.c -- fade out and paint the window black (win 0x004ab180), and the two startup splash screens that ride the same GWorldContainer/CopyBits plumbing as gworld.c. Notes: Research/notes/fadeblack.md */
#include "macport.h"
#include "gameutil.h"
#include "render.h"
#include "stellar.h"
#include "dialogs.h"
#include "game.h"

#define kSplashPictID       0x83
#define kAmbrosiaLogoPictID 0x1fa4

// FUNCTION: EVNOVA 0x004aaf60
// MAC: _ShowSplashScreen i386 0x00060e68 ppc 0x00068bfc
void ShowSplashScreen(void)
{
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kSplashPictID);
    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    rect = &loadPort.bounds;
    PaintRect(rect);
    box = loadPort.bounds;
    if (pic != 0) {
        CenterPict(pic, &box);
        DrawPicHandle((PicHandle)pic, &box);
        QDReleaseResource((PicHandle)pic);
    }
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             rect, GetCWindowPortRect(gWind), 0, 0);
    ScreenEndScene();
}


// FUNCTION: EVNOVA 0x004ab070
// MAC: _ShowAmbrosiaLogo i386 0x00060fb0 ppc 0x00068d54
void ShowAmbrosiaLogo(void)
{
    void *pic;
    Rect box;

    pic = GetPicture(kAmbrosiaLogoPictID);
    SetGWorldContainer(&loadPort);
    box = loadPort.bounds;
    CenterPict(pic, &box);
    ForeColor(blackColor);
    PaintRect(&loadPort.bounds);
    DrawPicHandle((PicHandle)pic, &box);
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
    box = *GetCWindowPortRect((WindowRef)gWind);
    CenterPict(pic, &box);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
    DrawPicHandle((PicHandle)pic, &box);
    if (pic != 0)
        QDReleaseResource((PicHandle)pic);
    FadeScreenIn();
}

// FUNCTION: EVNOVA 0x004ab180
// MAC: _HideAmbrosiaLogo i386 0x0006108f ppc 0x00068e40
void HideAmbrosiaLogo(void)
{
    FadeScreenOut();
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
}
