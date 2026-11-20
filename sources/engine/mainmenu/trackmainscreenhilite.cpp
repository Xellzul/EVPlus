/* trackmainscreenhilite.cpp -- MainScreenRolloverIdle; exe 0x0048c210. Notes: Research/notes/mainscreen.md */

#include <stddef.h>
#include "prototypes.h"
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
#define kHiliteStep      4



// FUNCTION: EVNOVA 0x0048c210
// MAC: _MainScreenRolloverIdle i386 0x00016462 ppc 0x0001a1f0
void MainScreenRolloverIdle(void)
{
    short i;
    short newIndex;
    Point pt;

    if (!inBackground) {
        if (slideIndex[0] < slideFrameCount[0] || slideIndex[1] < slideFrameCount[1]
            || slideIndex[2] < slideFrameCount[2]) {
            if (menuRolloverSprite != NULL)
                newIndex = 0;
            rolloverAlpha = 0;
        } else if (TickCount() > lastRolloverAnimTime) {
            if (menuRolloverSprite != NULL)
                newIndex = menuRolloverSprite->numFrames - 1;
            else
                newIndex = 0;
            lastRolloverAnimTime = TickCount();
            GetGlobalMouse(&pt);
            for (i = 0; i < kMainButtons; i++) {
                if (menuButtonSprite[i] != NULL && SWRLECollisionWithPoint(menuButtonSprite[i], &pt)) {
                    newIndex = i;
                    break;
                }
            }
            if (newIndex != rolloverIndex) {
                if (rolloverAlpha > 0)
                    rolloverAlpha -= kHiliteStep;
                else
                    rolloverIndex = newIndex;
            } else if (rolloverAlpha < kHiliteFull) {
                rolloverAlpha += kHiliteStep;
            }
            if (rolloverAlpha < 0)
                rolloverAlpha = 0;
            if (rolloverAlpha > kHiliteFull)
                rolloverAlpha = kHiliteFull;
        }
    } else {
        if (menuRolloverSprite != NULL)
            rolloverIndex = menuRolloverSprite->numFrames - 1;
        else
            rolloverIndex = 0;
        rolloverAlpha = kHiliteFull;
    }
}
