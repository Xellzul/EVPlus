// COMPILE: -lang c++ -bool off
/* showintroscreen.c -- ShowIntroScreen, MainScreenAppearStart, MainScreenAppear; exe 0x0048adc0..0x0048b250. Notes: Research/notes/mainscreen.md */

#include <stddef.h>
#include "prototypes.h"
#define kMainScreenSoundCount 4
#define kFirstMainScreenSound 0x258
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

#define kIntroPictSlots 4
#ifdef MACBUILD
#define kKeyReturn MacKey(0x24)
#define kKeySpace  MacKey(0x31)
#else
#define kKeyReturn 0x1c
#define kKeySpace  0x39
#endif



/* ShowIntroScreen. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048adc0
// MAC: _ShowIntroScreen i386 0x00016cf1 ppc 0x0001a9bc
void ShowIntroScreen(void)
{
    int32_t startTick;
    short i;
    Boolean pictSkipped;
    Boolean hit;
    PicHandle picHandle;
    void *snd;
    Rect rect;
    Point pt;

    HandleIntroMusic(1);
    FadeScreenOut();

    pictSkipped = 0;
    if (KeyCheck(keyPrefs.pause))
        pictSkipped = 1;

    if (!settings.windowed)
        HideTheCursor();

    for (i = 0; i < kIntroPictSlots; i++) {
        if (introPictID[i] > 0)
            picHandle = GetPicture(introPictID[i]);
        else
            picHandle = NULL;

        SetGWorldContainer(&loadPort);
        ForeColor(blackColor);
        PaintRect(&loadPort.bounds);

        rect = loadPort.bounds;

        if (picHandle != NULL) {
            CenterPict(picHandle, &rect);
            DrawPicHandle(picHandle, &rect);
            QDReleaseResource(picHandle);
        }

        UnsetGWorldContainer(&loadPort);

        SWSetPortToWindow(gSpriteWorld);
        ForeColor(blackColor);
        PaintRect(GetCWindowPortRect((WindowRef)gWind));
        CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &rect, &rect, 0, NULL);
        QDFlushPortBuffer(GetWindowPort(gWind), NULL);

        SWSetPortToWindow(gSpriteWorld);

        if (introPictID[i] > 0) {
            snd = ST_LoadSndResource(0x7533);
            if (snd != NULL) {
                PlaySound(snd, 0xa, gSoundVolume, gSoundVolume);
            }

            HandleIntroMusic(1);
            FadeScreenIn();

            while (Button() || KeyCheck(kKeyReturn) || KeyCheck(kKeySpace) || KeyCheck(keyPrefs.pause)) {
                BusyIdle();
                HandleIntroMusic(0);
            }

            hit = 0;
            startTick = ReadTick();

            while (ReadTick() - startTick < (introPictDelay[i] * 0x3c)
                   && !hit && !KeyCheck(kKeyReturn) && !KeyCheck(kKeySpace) && !pictSkipped) {
                BusyIdle();
                HandleIntroMusic(0);
                if (Button()) {
                    rect = *GetCWindowPortRect((WindowRef)gWind);
#if defined(MACBUILD) && (!defined(MAC1010) || TARGET_API_MAC_CARBON)
                    SectRect(&rect, &screenMinusDockRect, &rect);
#endif
                    GetGlobalMouse(&pt);
                    if (PtInRectPoint(pt, &rect)) {
                        hit = 1;
                    } else {
                        while (Button()) {
                            BusyIdle();
                            HandleIntroMusic(0);
                            if (ReadTick() - startTick > (introPictDelay[i] * 0x3c))
                                break;
                            if (KeyCheck(kKeyReturn) || KeyCheck(kKeySpace))
                                break;
                            if (KeyCheck(keyPrefs.pause)) {
                                pictSkipped = 1;
                                break;
                            }
                        }
                    }
                }
                if (KeyCheck(keyPrefs.pause))
                    pictSkipped = 1;
            }

            if (snd != NULL) {
                ST_HaltSound(snd);
                ST_DisposeSound(snd);
            }

            HandleIntroMusic(1);
            FadeScreenOut();

            if (pictSkipped)
                break;
        }
    }

    HandleIntroMusic(1);
    if (!pictSkipped) {
        StopIntroMusic(1);
        if (introTextID != -1) {
            ShowTheCursor();
            showingIntroText = 1;
            LoadDescription(briefingTextPtr, introTextID);
            MungeBriefing(0, -1);
            strcpy(textShowString, briefingTextPtr);
            ShowTextDialog(0, 0, 1);
            showingIntroText = 0;
        }
    }

    FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
}

/* the main screen's idle tick */
/* MainScreenAppearStart. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b230
// MAC: _MainScreenAppearStart i386 0x00018af1 ppc 0x0001c6f4
void MainScreenAppearStart(void)
{
    MainScreenAnimDraw();
    MainScreenUpdate(1);
#if V111
#else
    HandleIntroMusic(1);
    FadeScreenIn();
#endif
}

/* MainScreenAppear. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b250
// MAC: _MainScreenAppear i386 0x00016c7e ppc 0x0001a910
void MainScreenAppear(void)
{
    short i;

    for (i = 0; i < kMainScreenSoundCount; i++)
        mainScreenSnd[i] = ST_LoadSndResource((i + kFirstMainScreenSound));
}
