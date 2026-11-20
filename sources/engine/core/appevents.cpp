/* appevents.cpp -- the in-game event loop, app switching, window refresh and menu bar */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macport.h"
#include "scaleddraw.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "burger.h"
#include "sound.h"
#include "cefix.h"
#include "viewscale.h"
#include "uiscale.h"
#include "movie.h"
#include "ai.h"
#include "space.h"
#include "dialog.h"
#include "buttons.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00497500
// MAC: _IterateInGameEventLoop i386 0x00056859 ppc 0x0005ebe0
void IterateInGameEventLoop(short tickCount)
{
    Boolean exitLoop;
    Boolean depthMismatch;
    short i;
    Str255 str;

    exitLoop = 0;
    i = 0;
    do {
        HandleIntroMusic(0);
        ResetLastTick();
        WaitTicks((gameLoaded && inBackground) ? 30 : 0);
        i++;
        if (gNeedUpdateFlag) {
            depthMismatch = 0;
            if (((VideoColorDepth + 7) & ~7) != gSpriteWorld->pixelDepth)
                depthMismatch = 1;
            if (!depthMismatch) {
                SWSetPortToWindow(gSpriteWorld);
                if (!creatingPilotFile) {
                    if (gameInProgress && gameLoaded) {
                        if (onPlanet || inBackground) {
                            RedrawStatus(1);
                            RefreshSAT();
                        }
                    } else {
                        CopyBits(GWorldContainerBits(&loadPort),
                                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                                 GetCWindowPortRect(gWind), GetCWindowPortRect(gWind),
                                 0, 0);
                    }
                }
                SWSetPortToWindow(gSpriteWorld);
                gNeedUpdateFlag = 0;
            }
        } else if (!closeRequested) {
            exitLoop = 1;
        } else {
            closeRequested = 0;
            if (!screenFaded) {
                ShowTheCursor();
                GetIndString(str, kAlertStrList, 10);
                if (EVYesNoDialog(str))
                    gQuit = 1;
            }
        }
        if (tickCount > 0 && i >= tickCount)
            exitLoop = 1;
    } while (!exitLoop);
}

// FUNCTION: EVNOVA 0x00497cf0
// MAC: _SwitchedIn i386 0x000513a4 ppc 0x00059ca0
void SwitchedIn(EventRecord *ev)
{
    inBackground = 0;
    lastFPSUpdateTime = 0;
    fpsCounter = 0;
    gSpeedMult = 1.0;
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    if (introMusicMovie != NULL)
        MovieStart(introMusicMovie);
}

// FUNCTION: EVNOVA 0x00497d50
// MAC: _SwitchedOut i386 0x0004f381 ppc 0x00057ff8
void SwitchedOut(EventRecord *ev)
{
    void *snd;

    inBackground = 1;
    if (introMusicMovie != NULL)
        MovieStop(introMusicMovie);
    if (warpUpSnd[0] != NULL && ST_IsSoundPlaying(warpUpSnd[0]) != 0)
        ST_HaltSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL && ST_IsSoundPlaying(snd) != 0)
        ST_HaltSound(warpUpSnd[1]);
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    BackColor(whiteColor);
}

/*
 * SpriteWorld's +0x1c is windowFrameP, so the destination rect is the same
 * frameRect member reached through the other frame.
 */

// FUNCTION: EVNOVA 0x00497df0
// MAC: _RefreshWindowBuffer i386 0x00051420 ppc 0x00059d54
void RefreshWindowBuffer(void)
{
    SWSetPortToWorkArea(gSpriteWorld);
    if (gameLoaded && gameInProgress)
        SetSystemBackgroundColor();
    else
        ForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    CopyBits(GetPortBitMapForCopyBits(
                 gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(gSpriteWorld->windowFrameP->framePort),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
    View::Flush();
    ScreenEndScene();
}

// FUNCTION: EVNOVA 0x0049d7a0
void HideTheMenuBar(void)
{
}

// FUNCTION: EVNOVA 0x0049d7b0
void ShowTheMenuBar(void)
{
}

EXTERN_C_END
