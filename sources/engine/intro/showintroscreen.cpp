/* showintroscreen.cpp -- ShowIntroScreen, MainScreenAppearStart, MainScreenAppear; exe 0x0048adc0..0x0048b250. Notes: Research/notes/mainscreen.md */

#define kMainScreenSoundCount 4
#define kFirstMainScreenSound 0x258
#include "dialogs.h"
#include "sound.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "game.h"
#include "missions.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

#define kIntroPictSlots 4



// FUNCTION: EVNOVA 0x0048adc0
// MAC: _ShowIntroScreen i386 0x00016cf1 ppc 0x0001a9bc
void ShowIntroScreen(void)
{
    int32_t startTick;
    short i;
    Boolean pictSkipped;
    Boolean hit;
    PicHandle picHandle;
    Sound *snd;
    Rect rect;
    Point pt;

    HandleIntroMusic(1);
    FadeScreenOut();

    pictSkipped = 0;
    if (KeyCheck(keyPrefs.pause))
        pictSkipped = 1;

    if (!settings.windowed)
        HideTheCursor();

    {
        UI::MenuScale menu;
        for (i = 0; i < kIntroPictSlots; i++) {
            if (introPictID[i] > 0)
                picHandle = GetPicture(introPictID[i]);
            else
                picHandle = NULL;

            rect = PaintLoadPagePicture(picHandle);
            if (picHandle != NULL)
                QDReleaseResource(picHandle);

            ShowLoadPage(rect, rect);
            Screen::Changed();
            Screen::EndFrame();

            if (introPictID[i] > 0) {
                snd = ST_LoadSndResource(0x7533);
                if (snd != NULL) {
                    PlaySound(snd, 0xa, gSoundVolume, gSoundVolume);
                }

                HandleIntroMusic(1);
                FadeScreenIn();

                while (Button() || KeyCheck(kScanReturn) || KeyCheck(kScanSpace) || KeyCheck(keyPrefs.pause)) {
                    BusyIdle();
                    HandleIntroMusic(0);
                }

                hit = 0;
                startTick = ReadTick();

                while (ReadTick() - startTick < (introPictDelay[i] * 0x3c)
                       && !hit && !KeyCheck(kScanReturn) && !KeyCheck(kScanSpace) && !pictSkipped) {
                    BusyIdle();
                    HandleIntroMusic(0);
                    if (Button()) {
                        rect = *GetCWindowPortRect((WindowRef)gWind);
                        GetGlobalMouse(&pt);
                        if (PtInRectPoint(pt, &rect)) {
                            hit = 1;
                        } else {
                            while (Button()) {
                                BusyIdle();
                                HandleIntroMusic(0);
                                if (ReadTick() - startTick > (introPictDelay[i] * 0x3c))
                                    break;
                                if (KeyCheck(kScanReturn) || KeyCheck(kScanSpace))
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
    }

    HandleIntroMusic(1);
    if (!pictSkipped) {
        StopIntroMusic(1);
        if (introTextID != -1) {
            ShowTheCursor();
            showingIntroText = 1;
            LoadDescription(briefingTextPtr, introTextID);
            MungeBriefing(0, -1);
            CopyBriefingText(textShowString, briefingTextPtr);
            ShowTextDialog(0, 1);
            showingIntroText = 0;
        }
    }

    FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
}

/* the main screen's idle tick */
// FUNCTION: EVNOVA 0x0048b230
// MAC: _MainScreenAppearStart i386 0x00018af1 ppc 0x0001c6f4
void MainScreenAppearStart(void)
{
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    HandleIntroMusic(1);
    FadeScreenIn();
}

// FUNCTION: EVNOVA 0x0048b250
// MAC: _MainScreenAppear i386 0x00016c7e ppc 0x0001a910
void MainScreenAppear(void)
{
    short i;

    for (i = 0; i < kMainScreenSoundCount; i++)
        mainScreenSnd[i] = ST_LoadSndResource((i + kFirstMainScreenSound));
}
