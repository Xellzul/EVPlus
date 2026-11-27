/* dlgsound.cpp -- the dialog sound bank's stop-all and its two-part chime (win 0x0048bbb0 and 0x0048bc20). Notes: Research/notes/dlgsound.md */


#include "render.h"
#include "game.h"
#include "viewscale.h"
#include "uiscale.h"
#include "gameutil.h"
#include "sound.h"
#include "crashlog.h"
#include "dialogs.h"
#include "painter.h"



// FUNCTION: EVNOVA 0x0048bbb0
// MAC: _DisposeMainScreenSounds i386 0x00016cae ppc 0x0001a960
void DisposeMainScreenSounds(void)
{
    short i;

    for (i = 0; i < 4; i++) {
        if (mainScreenSnd[i] != NULL) {
            while (ST_IsSoundPlaying(mainScreenSnd[i]) != 0)
                BusyIdle();
            ST_HaltSound(mainScreenSnd[i]);
            ST_DisposeSound(mainScreenSnd[i]);
            mainScreenSnd[i] = NULL;
        }
    }
}

// FUNCTION: EVNOVA 0x0048bc20
// MAC: _PlayMainScreenClickSound i386 0x000177af ppc 0x0001b38c
void PlayMainScreenClickSound(void)
{
    PlaySound(mainScreenSnd[0], 10, gSoundVolume, gSoundVolume);
    while (ST_IsSoundPlaying(mainScreenSnd[0]) != 0)
        BusyIdle();
    PlaySound(mainScreenSnd[1], 10, gSoundVolume, gSoundVolume);
}





/* the logo, rollover and button slides, whole, where the menu's picture lies (menuTopCorner) */
void PlaceMainScreenAnim(void)
{
    Rect rollover = { 0, 0, 0, 0 };

    if (menuRolloverSprite != NULL && menuRolloverSprite->numFrames > 0)
        rollover = menuRolloverSprite->frameArray[0]->frameRect;
    SetRect(&logoDestRect, menuTopCorner.h + logoDestPoint.h, menuTopCorner.v + logoDestPoint.v,
            menuTopCorner.h + logoDestPoint.h + gMainPicWidthDelta,
            menuTopCorner.v + logoDestPoint.v + gMainPicHeightDelta);
    SetRect(&rolloverDestRect, menuTopCorner.h + rolloverDestPoint.h, menuTopCorner.v + rolloverDestPoint.v,
            menuTopCorner.h + rolloverDestPoint.h + (rollover.right - rollover.left),
            menuTopCorner.v + rolloverDestPoint.v + (rollover.bottom - rollover.top));
    for (short i = 0; i < 3; i++)
        SetRect(&slideDestRect[i], menuTopCorner.h + slideDestPoint[i].h, menuTopCorner.v + slideDestPoint[i].v,
                menuTopCorner.h + slideDestPoint[i].h + slideFrameSize[i].width,
                menuTopCorner.v + slideDestPoint[i].v + slideFrameSize[i].height);
}

// FUNCTION: EVNOVA 0x0048bc90
// MAC: _LoadMainScreenAnim i386 0x0001781d ppc 0x0001b404
void LoadMainScreenAnim(void)
{
    short spriteID;
    short xTiles;
    short i;
    uint32_t tick;

    if (GetSpriteInfo(0x25e, &spriteID, NULL, &gMainPicWidthDelta, &gMainPicHeightDelta,
                       &xTiles, &logoFrameCount)) {
        if (xTiles != 1)
            crashlog("warning: logo animation image has wrong dimensions");
        CreateGWorldContainerFromPictResource(&logoPort, 0, spriteID);
    }

    for (i = 0; i < 3; i++) {
        if (GetSpriteInfo(0x260 + i, &spriteID, NULL, &slideFrameSize[i].width, &slideFrameSize[i].height,
                           &xTiles, &slideFrameCount[i])) {
            if (xTiles != 1)
                crashlog("warning: button slide animation image has wrong dimensions");
            CreateGWorldContainerFromPictResource(&slidePort[i], 0, spriteID);
        }
        if (inBackground) {
            slideIndex[i] = slideFrameCount[i];
        } else if (i == 0) {
            slideIndex[i] = -1;
        } else {
            slideIndex[i] = (slideIndex[i - 1] - slideFrameCount[i - 1] * 0.5);
        }
    }

    if (inBackground) {
        if (menuRolloverSprite != NULL)
            rolloverIndex = menuRolloverSprite->numFrames - 1;
        else
            rolloverIndex = 0;
        rolloverAlpha = 0x20;
    } else {
        rolloverAlpha = 0;
    }

    PlaceMainScreenAnim();                  /* the frame sizes are known now */
    tick = ReadTick();
    lastLogoAnimTime = lastSlideAnimTime = tick;
    lastRolloverAnimTime = 0;
}

// FUNCTION: EVNOVA 0x0048bf40
// MAC: _DisposeMainScreenAnim i386 0x00017a81 ppc 0x0001b6ec
void DisposeMainScreenAnim(void)
{
    short i;

    if (logoPort.world != NULL) {
        DisposeGWorldContainer(&logoPort);
        logoPort.world = NULL;
    }
    for (i = 0; i < 3; i++) {
        if (slidePort[i].world) {
            DisposeGWorldContainer(&slidePort[i]);
            slidePort[i].world = NULL;
        }
    }
}



// FUNCTION: EVNOVA 0x0048bfb0
// MAC: _MainScreenAnimIdle i386 0x0001883c ppc 0x0001c414
void MainScreenAnimIdle(void)
{
    UI::MenuScale menu;
    short i;
    short oldFrame;

    if (!inBackground) {
        if (ReadTick() >= lastLogoAnimTime + 5) {
            lastLogoAnimTime = ReadTick();
            oldFrame = logoIndex;
            do {
                logoIndex = Rand(logoFrameCount);
            } while (logoIndex == oldFrame);
        }
    }

    if (!inBackground) {
        if (ReadTick() >= lastSlideAnimTime + 1) {
            lastSlideAnimTime = ReadTick();
            for (i = 0; i < 3; i++) {
                if (slideIndex[i] < slideFrameCount[i]) {
                    if (slideIndex[i] == 0)
                        PlaySound(mainScreenSnd[2], 0x32, gSoundVolume, gSoundVolume);
                    slideIndex[i]++;
                    if (slideIndex[i] == slideFrameCount[i]) {
                        PlaySound(mainScreenSnd[3], 0x32, gSoundVolume, gSoundVolume);
                        if (i == 2)
                            MainScreenUpdate(1);
                    }
                }
            }
        }
    }

    MainScreenRolloverIdle();
    MainScreenAnimDraw();
    MainScreenUpdate(2);

    UI::Painter screen(UI::Surface::Screen());

    screen.Copy(UI::Surface::Load(), logoDestRect, logoDestRect);
    screen.Copy(UI::Surface(View::UIWork()), rolloverDestRect, rolloverDestRect);

    for (i = 0; i < 3; i++) {
        if (slidePort[i].world != NULL && slideIndex[i] < 0x7d00) {
            screen.Copy(UI::Surface(View::UIWork()), slideDestRect[i], slideDestRect[i]);
            if (slideIndex[i] >= slideFrameCount[i])
                slideIndex[i] = 0x7d00;
        }
    }

    /* the animation and the rollover move by ticks: shown once a tick, not on every pass of the menu loop */
    static Word32 lastShown;
    if (ReadTick() != lastShown) {
        lastShown = ReadTick();
        ScreenEndScene();
    }
}
