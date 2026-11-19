// COMPILE: -lang c++ -bool off
/* gameview.c -- centre the 640x480 play area in the window (win 0x00486790). Notes: Research/notes/gameview.md */
#include <stddef.h>

#include "macport.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "render.h"
#include "sound.h"

#define kViewWidth  640
#define kViewHeight 480
#define kViewSlots  5


#define kIntroDescID   0x7fff

/*
 * Both log calls are stubs in the shipped build, so holding shift only
 * suppresses the briefing.
 */
// FUNCTION: EVNOVA 0x00486120
// MAC: _DoAboutBox i386 0x00015fb7 ppc 0x00019cc8
void DoAboutBox(void)
{
    StopIntroMusic(1);
    if ((KeyCheck(kScanLShift) != 0 || KeyCheck(kScanRShift) != 0)
        && didSpeak == 0 && gPause == 0) {
        CheckSpeechMgr();
        SpeakIt("Greetings professor Follkinn. Would you like to play a game?", 1);
        didSpeak = 1;
        return;
    } else {
        mainScreenBackgroundDirty = 1;
        LoadDescription(briefingTextPtr, kIntroDescID);
        MungeBriefing(0, -1);
        strcpy(textShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}

/* --- TrackMainScreenButtons (0x004861b0) --- */

/* h first, v second -- NOT a Point, whose v comes first */


#define kButtonDownSnd 0
#define kButtonUpSnd   1
#define kSoundChannel  10

/*
 * Returns the button under the mouse at release, or -1. Each redraw reads the
 * frame rect out of buttonArray[0] even when it draws buttonArray[1], because
 * the two frames share a rect.
 */
// FUNCTION: EVNOVA 0x004861b0
// MAC: _TrackMainScreenButtons i386 0x000195b9 ppc 0x0001d30c
short TrackMainScreenButtons(Point where)
{
    Rect box;
    Point pt;
    short i;
    short over;
    short hit;

    SWSetPortToWindow(gSpriteWorld);
    for (i = 0; i < kMainButtons; i++)
        SWSetSpriteLocation(menuButtonSprite[i],
                            (menuTopCorner.h + menuButtonPos[i].h),
                            (menuTopCorner.v + menuButtonPos[i].v));
    hit = -1;
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i] != NULL
            && SWRLECollisionWithPoint(menuButtonSprite[i], &where)) {
            hit = i;
            break;
        }
    }
    if (hit != -1) {
        PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
        for (i = 0; i < kMainButtons; i++) {
            if (menuButtonSprite[i] != NULL) {
                box = menuButtonSprite[i]->frameArray[0]->frameRect;
                OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                           menuTopCorner.v + menuButtonPos[i].v);
                SWSetSpriteLocation(menuButtonSprite[i],
                                    (menuTopCorner.h + menuButtonPos[i].h),
                                    (menuTopCorner.v + menuButtonPos[i].v));
                if (hit == i)
                    BlitPixieRLEDrawProc(menuButtonSprite[i]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                else
                    BlitPixieRLEDrawProc(menuButtonSprite[i]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                ForeColor(blackColor);
                CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                         GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                         &box, &box, 0, NULL);
                QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
            }
        }
        SWSetPortToWindow(gSpriteWorld);
        while (Button()) {
            BusyIdle();
            HandleIntroMusic(0);
            GetGlobalMouse(&pt);
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            MainScreenRolloverIdle();
            MainScreenRolloverDraw();
            CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            over = -1;
            for (i = 0; i < kMainButtons; i++) {
                if (menuButtonSprite[i] != NULL
                    && SWRLECollisionWithPoint(menuButtonSprite[i], &pt)) {
                    over = i;
                    break;
                }
            }
            if (over != hit) {
                if (over <= kMainButtons - 1 && over >= 0) {
                    PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                    box = menuButtonSprite[over]->frameArray[0]->frameRect;
                    OffsetRect(&box, menuTopCorner.h + menuButtonPos[over].h,
                               menuTopCorner.v + menuButtonPos[over].v);
                    SWSetSpriteLocation(menuButtonSprite[over],
                                        (menuTopCorner.h + menuButtonPos[over].h),
                                        (menuTopCorner.v + menuButtonPos[over].v));
                    BlitPixieRLEDrawProc(menuButtonSprite[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                             &box, &box, 0, NULL);
                    QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
                } else {
                    PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                }
                if (hit <= kMainButtons - 1 && hit >= 0) {
                    box = menuButtonSprite[hit]->frameArray[0]->frameRect;
                    OffsetRect(&box, menuTopCorner.h + menuButtonPos[hit].h,
                               menuTopCorner.v + menuButtonPos[hit].v);
                    SWSetSpriteLocation(menuButtonSprite[hit],
                                        (menuTopCorner.h + menuButtonPos[hit].h),
                                        (menuTopCorner.v + menuButtonPos[hit].v));
                    BlitPixieRLEDrawProc(menuButtonSprite[hit]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                             &box, &box, 0, NULL);
                    QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
                }
            }
            hit = over;
        }
    }
    if (hit != -1)
        PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
    return hit;
}

// FUNCTION: EVNOVA 0x00486790
// MAC: _InitMainScreen i386 0x00016071 ppc 0x00019d9c
void InitMainScreen(void)
{
    short i;

    didSpeak = 0;
    onPlanet = 0;
    doMainScreenAnim = 1;

    standardPicRect.left = (GetCWindowPortRect((WindowRef)gWind)->left
                      + GetCWindowPortRect((WindowRef)gWind)->right) / 2 - kViewWidth / 2;
    standardPicRect.top = (GetCWindowPortRect((WindowRef)gWind)->top
                     + GetCWindowPortRect((WindowRef)gWind)->bottom) / 2 - kViewHeight / 2;
    standardPicRect.right = standardPicRect.left + kViewWidth;
    standardPicRect.bottom = standardPicRect.top + kViewHeight;

    introTextID = -1;
    for (i = 0; i < 4; i++)
        introPictID[i] = -1;

    creatingPilotFile = 0;
    majorPrefsChange = 0;
}
