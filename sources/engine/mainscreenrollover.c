// COMPILE: -lang c++ -bool off
/* mainscreenrollover.c -- MainScreenRolloverDraw; exe 0x0048c580..0x0048c720. Notes: Research/notes/mainscreen.md */

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
/* the main-screen object's globals; this file ends that object (the .CRT hook) */
PicHandle keyPic;
short keyDialogSel;
short introTextID;
short introPictID[4];
short introPictDelay[4];
DialogPtr prefsDialog;
DialogPtr keyDialog;
Rect standardPicRect;
Rect interfacePicRect;
Rect logoDestRect;
Rect rolloverDestRect;
Rect slideDestRect[3];
Boolean didSpeak;
Boolean doMainScreenAnim;
#if !defined(MACBUILD) || defined(MAC1010)
Boolean majorPrefsChange;
#endif
Boolean creatingPilotFile;
Boolean gUpdateFlag;
Boolean regNoticeShown;
Boolean showingIntroText;
Str255 newPlayerName[2];
Str255 newPilotPopupChoice[2];
void *mainScreenSnd[4];
ShortPoint mainMenuCenter;
ShortPoint menuButtonPos[6];
GWorldContainer logoPort;
GWorldContainer slidePort[3];
short logoIndex;
short rolloverIndex;
short slideIndex[3];
short rolloverAlpha;
short gMainPicWidthDelta;
short gMainPicHeightDelta;
ShortPoint logoDestPoint;
ShortPoint rolloverDestPoint;
AnimDelta slideFrameSize[3];
ShortPoint slideDestPoint[3];
ShortPoint menuTopCorner;
short logoFrameCount;
short slideFrameCount[3];
uint32_t lastLogoAnimTime;
uint32_t lastRolloverAnimTime;
uint32_t lastSlideAnimTime;
Boolean mainScreenBackgroundDirty;

/* MainScreenRolloverDraw. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c580
// MAC: _MainScreenRolloverDraw i386 0x00017ad4 ppc 0x0001b76c
void MainScreenRolloverDraw(void)
{
    short idx;
    Rect srcRect;

    if (menuRolloverSprite != NULL) {
        idx = rolloverIndex;
        if (rolloverIndex < 0)
            idx = 0;
        if (idx >= menuRolloverSprite->numFrames)
            idx = menuRolloverSprite->numFrames - 1;
        srcRect = menuRolloverSprite->frameArray[idx]->frameRect;
        rolloverDestRect.right = rolloverDestRect.left + (srcRect.right - srcRect.left);
        rolloverDestRect.bottom = rolloverDestRect.top + (srcRect.bottom - srcRect.top);
        SWSetSpriteLocation(menuRolloverSprite, rolloverDestRect.left,
                            rolloverDestRect.top);
        menuRolloverSprite->translucency = kHiliteFull;
        menuRolloverSprite->tintRed = rolloverAlpha;
        menuRolloverSprite->tintGreen = rolloverAlpha;
        menuRolloverSprite->tintBlue = rolloverAlpha;
        menuRolloverSprite->tintMode = 0;
        menuRolloverSprite->tintColor = 0;
        gSWCurrentSpriteBeingDrawn = menuRolloverSprite;

        if (gSpriteWorld->pixelDepth == kDepth16) {
            BlitPixieRLETranslucentDrawProc(menuRolloverSprite->frameArray[idx],
                gSpriteWorld->workFrameP, &srcRect, &rolloverDestRect);
        } else {
            BlitPixieRLEDrawProc(menuRolloverSprite->frameArray[idx],
                gSpriteWorld->workFrameP, &srcRect, &rolloverDestRect);
        }
    }
}
