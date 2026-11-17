// COMPILE: -lang c++ -bool off
/* mainscreen.c -- the main screen: DoMainScreen, its choice and keyboard handlers (win 0x00486880..0x004872a0). Notes: Research/notes/mainscreen.md */
/*
 * The checks are a straight run of independent ifs rather than a chain, so each
 * handled key falls through to the remaining comparisons.
 */
#include <stddef.h>

#include "prototypes.h"

#define kMainScreenNew      0
#define kMainScreenOpen     1
#define kMainScreenQuit     2
#define kMainScreenEnter    3
#define kMainScreenPrefs    4
#define kMainScreenAbout    5

#ifdef MACBUILD
#define kKeyS               MacKey(0x01)
#define kKeyL               MacKey(0x25)
#define kKeyE               MacKey(0x0e)
#else
#define kKeyS               0x1f
#define kKeyL               0x26
#endif
#define kCreditsTextID      0x7ffe

#include "resources.h"
#include "shipobj.h"
#include "gametypes.h"
#include "gameconst.h"
#include "macport.h"
#include "dialogs.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "crashlog.h"
#include "game.h"

#define kStrListMainScreen   0x8c
#define kStrNewPilotConfirm     8
#define kStrMissingResource  0x34

/* ---- DoMainScreen (0x00486880) ---- */

#ifndef MACBUILD
uint32_t LastTick;                   /* 0x008700f0 */
#endif

/* 0x0058f8ac, 0x0058f8b0 and 0x0058f710 are IAT slots, not variables:
   dllimport is what makes the call go through them. */

#define kIdleWaitTicks       30
#define kStrQuitConfirm     0xa
#define kStartupIterations 0x32

// FUNCTION: EVNOVA 0x00486880
// MAC: _DoMainScreen i386 0x0001af02 ppc 0x0001ee70
void DoMainScreen(void)
{
    int32_t mouseV;
    Str255 confirm;
    int32_t mouseH;
    Point where;
    short delay;
    int key;
    int upper;
    int i;
    int buttons;
    Boolean animCut;

    crashlog("preparing main screen");
    mainScreenBackgroundDirty = 0;
    showingIntroText = 0;
    logoPort.world = NULL;
    logoIndex = 0;
    if (menuRolloverSprite != NULL)
        rolloverIndex = menuRolloverSprite->numFrames - 1;
    else
        rolloverIndex = 0;
    rolloverAlpha = 0;
    slidePort[0].world = NULL;
    slideIndex[0] = 0;
    slidePort[1].world = NULL;
    slideIndex[1] = 0;
    slidePort[2].world = NULL;
    slideIndex[2] = 0;
    mainScreenSnd[0] = NULL;
    mainScreenSnd[1] = NULL;
    mainScreenSnd[2] = NULL;
    mainScreenSnd[3] = NULL;
    crashlog("setting up world");

    if (gameInProgress) {
        DeleteAllComputerShips(0, 1);
        PropagateMissionBitEffects();
        SetupShipsInSystem(shipData->curSystem, 1);
        CreateAsteroids();
        ExploreNebulaeFromSystem(shipData->curSystem);
        HandleStellarSystemVisibility();
        MissionPostLoadValidCheck();
        onPlanet = 0;
        killShots = killBoxes = killExplods = killSmoke = killAsteroids = 1;
        missionSoundID = -1;
        SWSetPortToBackground(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWorkArea(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
    }

    crashlog("setting up main screen graphics");
    SetupInterfaceBackground();
    ShowTheCursor();
    crashlog("doing main screen");
    LoadMainScreenAnim();
    MainScreenAnimDraw();
    MainScreenAppearStart();
    MainScreenAppear();
    SWSetPortToWindow(gSpriteWorld);
    GetCWindowPortRect(gWind);
    gNeedUpdateFlag = 1;
    regNoticeShown = 1;

    GetCWindowPortRect(gWind);
    gNeedUpdateFlag = 1;
    AdjustSoundVolume();
    ShowTheCursor();
    IterateInGameEventLoop(kStartupIterations);
    gUpdateFlag = 0;

    while (gQuit == 0) {
        if (gUpdateFlag != 0)
            MainScreenUpdate(1);

        delay = (slideIndex[0] < kAnimDone || slideIndex[1] < kAnimDone
                 || slideIndex[2] < kAnimDone)
                ? 0
                : ((gameLoaded != 0 && inBackground != 0)
                   ? kIdleWaitTicks : 0);
        if (delay != 0) {
            LastTick = ReadTick();
            WaitTicks(delay);
        }

        key = KeyboardGet();
        upper = toupper(key);
        buttons = MouseReadButtons();
        MainScreenIdle();
        if (buttons != 0) {
            animCut = 0;
            MouseClicked = 0;
            i = 0;
            do {
                if (slideIndex[(short)i] < kAnimDone) {
                    slideIndex[(short)i] = kAnimDone;
                    animCut = 1;
                }
                i = i + 1;
            } while ((short)i < 3);
            SWSetPortToWindow(gSpriteWorld);
            MouseReadPosition(&mouseH, &mouseV);
            if (animCut) {
                SetupInterfaceBackground();
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            } else {
                MainScreenAnimDraw();
                MainScreenUpdate(0);
                ScreenEndScene();
            }
            where.h = mouseH;
            where.v = mouseV;
            MainScreenChoice(TrackMainScreenButtons(where));
        } else if (key != 0) {
            MainScreenKey(upper, 0, 0);
        }

        if (gNeedUpdateFlag != 0) {
            MainScreenAnimDraw();
            MainScreenUpdate(1);
            gNeedUpdateFlag = 0;
        }
        if (closeRequested != 0) {
            closeRequested = 0;
            if (screenFaded == 0) {
                ShowTheCursor();
                GetIndString(confirm, kStrListMainScreen, kStrQuitConfirm);
                if (EVYesNoDialog(confirm))
                    gQuit = 1;
            }
        }
    }

    DisposeMainScreenAnim();
    DisposeMainScreenSounds();
}

// FUNCTION: EVNOVA 0x00486ec0
void FUN_00486ec0(void)
{
}

// FUNCTION: EVNOVA 0x00486ed0
// MAC: _MainScreenChoice i386 0x0001a72a ppc 0x0001e614
void MainScreenChoice(short which)
{
    Boolean confirmed;
    Boolean missingResource;
    unsigned char msg[256];
    short i;
    short j;

    if (which == kMainScreenNew) {
        if (gameInProgress) {
            GetIndString(msg, kStrListMainScreen, kStrNewPilotConfirm);
            confirmed = EVYesNoDialog(msg);
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        } else {
            confirmed = 1;
        }
        if (confirmed) {
            DoNewPilot();
            SWSetPortToWindow(gSpriteWorld);
            InvalWindowPortRect((WindowRef)gWind);
            SWHideAllSprites(gSpriteWorld);
        }
    }

    if (which == kMainScreenOpen) {
        crashlog("opening pilot file");
        if (OpenPilotFile((char *)&missingResource)) {
            if (missingResource) {
                crashlog("showing missing resource warning");
                MainScreenAnimDraw();
                MainScreenUpdate(1);
                GetIndString(msg, kStrListMainScreen, kStrMissingResource);
                EVNoteAlert(msg);
            }
            crashlog("preparing universe");
            gameInProgress = 1;
            InvalWindowPortRect((WindowRef)gWind);
            DeleteAllComputerShips(0, 1);
            PropagateMissionBitEffects();
            SetupShipsInSystem(shipData->curSystem, 1);
            HandleStellarSystemVisibility();
            CreateAsteroids();

            for (i = 0; i < 0x800; i++) {
                if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }

            crashlog("preparing missions");
            for (j = 0; j < 0x10; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionInfiniteAuxShips) != 0)
                        currentMissions[j].auxShipsLeft = currentMissions[j].auxShipCount;
                    if (currentMissions[j].auxShipsLeft > 0) {
                        currentMissions[j].auxShipDelay = (Rand(0x46) + 0x46);
                        currentMissions[j].auxShipsSpawned = 0;
                    }
                }
            }

            onPlanet = 0;
            killShots = killBoxes = killExplods = killSmoke = killAsteroids = 1;
            missionSoundID = -1;
            SWHideAllSprites(gSpriteWorld);
            crashlog("finished opening pilot file");
        }
    }

    if (which == kMainScreenQuit) {
        Delay(0xf, &tix);
        gQuit = 1;
    }

    if (which == kMainScreenEnter) {
        if (gameInProgress) {
            if (!IsDying(shipData)) {
#if V111
#ifdef MACBUILD
                FlushKeys();
#endif
#endif
                EnterGameFromMainScreen();
            } else {
                PlaySound(beepSnd[3], 0xa, gSoundVolume, gSoundVolume);
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            }
        } else {
            PlaySound(beepSnd[3], 0xa, gSoundVolume, gSoundVolume);
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        }
    }

    if (which == kMainScreenPrefs) {
        DoPrefsDialog();
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        SWSetPortToWindow(gSpriteWorld);
        InvalWindowPortRect((WindowRef)gWind);
        AdjustSoundVolume();
    }

    if (which == kMainScreenAbout || which == 6) {
        DoAboutBox();
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        SWSetPortToWindow(gSpriteWorld);
        InvalWindowPortRect((WindowRef)gWind);
    }

    if (!gQuit) {
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
}


// FUNCTION: EVNOVA 0x004872a0
// MAC: _MainScreenKey i386 0x0001aba0 ppc 0x0001eabc
#ifdef MACBUILD
void MainScreenKey(char key, unsigned char code, short modifiers)
{
    key = tolower(key);
    if (key == 'n') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenNew);
    }
    if (key == 'o') {
        if (KeyCheck(kKeyS) == 0 && KeyCheck(kKeyL) == 0) {
            PlayMainScreenClickSound();
            MainScreenChoice(kMainScreenOpen);
        }
    }
    if (key == 'q') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenQuit);
    }
    if (key == 'e') {
#ifndef MAC1010
        do
            QuickKeyCheckHandler();
        while (QuickKeyCheck(kKeyE));
#endif
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenEnter);
    }
    if (key == 'p') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenPrefs);
    }
    if (key == 'a') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenAbout);
    }
    if (key == 'x') {
        PlayMainScreenClickSound();
        mainScreenBackgroundDirty = 1;
        LoadDescription(briefingTextPtr, kCreditsTextID);
        MungeBriefing(0, -1);
        strcpy(textShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}
#else
void MainScreenKey(char key, int32_t a, int32_t b)
{
    key = tolower(key);
    if (key == 'n') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenNew);
    }
    if (key == 'o') {
        if (KeyCheck(kKeyS) == 0 && KeyCheck(kKeyL) == 0) {
            PlayMainScreenClickSound();
            MainScreenChoice(kMainScreenOpen);
        }
    }
    if (key == 'q') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenQuit);
    }
    if (key == 'e') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenEnter);
    }
    if (key == 'p') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenPrefs);
    }
    if (key == 'a') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenAbout);
    }
    if (key == 'x') {
        PlayMainScreenClickSound();
        mainScreenBackgroundDirty = 1;
        LoadDescription(briefingTextPtr, kCreditsTextID);
        MungeBriefing(0, -1);
        strcpy(textShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}
#endif

