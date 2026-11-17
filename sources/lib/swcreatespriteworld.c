// COMPILE: -lang c++ -bool off
/* swcreatespriteworld.c -- SWEnterSpriteWorld (SpriteWorld.c's order) and SWCreateSpriteWorld (win 0x00477640, donor SpriteWorld.c line 117). Notes: Research/notes/swcreatespriteworld.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"

/* backgroundColor is three shorts at +0xdf, +0xe1, +0xe3 -- ODD offsets, which x86
   natural alignment cannot produce, so it cannot be declared as a member here.
   Reached the way spriteworld.c's SWSetBackgroundColor reaches the same three. */
#define backgroundRed(sw)   (*(unsigned short *)((char *)(sw) + 0xdf))
#define backgroundGreen(sw) (*(unsigned short *)((char *)(sw) + 0xe1))
#define backgroundBlue(sw)  (*(unsigned short *)((char *)(sw) + 0xe3))

// FUNCTION: EVNOVA 0x00477630
// MAC: _SWEnterSpriteWorld i386 0x000b6b30 ppc 0x000c2630
int SWEnterSpriteWorld(void)
{
    SWSetStickyIfError(0);
    return 0;
}

// FUNCTION: EVNOVA 0x00477640
// MAC: _SWCreateSpriteWorld i386 0x000b6b92 ppc 0x000c26d8
short SWCreateSpriteWorld(void *mainGDH, SpriteWorldRec **spriteWorldP,
                          SWFrameRec *windowFrameP, SWFrameRec *backFrameP,
                          SWFrameRec *workFrameP, short maxDepth)
{
    short err;
    SpriteWorldRec *tempSpriteWorldP;

    err = noErr;
    *spriteWorldP = 0;

    tempSpriteWorldP = (SpriteWorldRec *)AllocAPointerClear(sizeof(SpriteWorldRec));
    if (tempSpriteWorldP == 0)
        err = memFullErr;

    if ((int)err == noErr) {
        err = SWCreateSpriteLayer(&tempSpriteWorldP->deadSpriteLayerP);

        if ((int)err != noErr)
            DeallocAPointer(tempSpriteWorldP);
    }

    if ((int)err == noErr) {
        tempSpriteWorldP->pixelDepth = 16;

        tempSpriteWorldP->headSpriteLayerP = 0;
        tempSpriteWorldP->tailSpriteLayerP = 0;

        tempSpriteWorldP->mainSWGDH = mainGDH;
        tempSpriteWorldP->windowFrameP = windowFrameP;
        tempSpriteWorldP->extraBackFrameP = 0;
        tempSpriteWorldP->backFrameP = backFrameP;
        tempSpriteWorldP->workFrameP = workFrameP;
        tempSpriteWorldP->offscreenDrawProc = SWStdWorldDrawProc;
        tempSpriteWorldP->screenDrawProc = SWStdWorldDrawProc;
        tempSpriteWorldP->doubleRectDrawProc = 0;
        tempSpriteWorldP->postEraseCallBack = 0;
        tempSpriteWorldP->postDrawCallBack = 0;

        tempSpriteWorldP->windRect = windowFrameP->frameRect;

        tempSpriteWorldP->backRect = backFrameP->frameRect;

        tempSpriteWorldP->originalWindRect = windowFrameP->frameRect;

        tempSpriteWorldP->originalBackRect = backFrameP->frameRect;

        tempSpriteWorldP->headUpdateRectP = 0;

        tempSpriteWorldP->scrollRectMoveBounds.left = 0;
        tempSpriteWorldP->scrollRectMoveBounds.top = 0;
        tempSpriteWorldP->scrollRectMoveBounds.right = 32767;
        tempSpriteWorldP->scrollRectMoveBounds.bottom = 32767;
        tempSpriteWorldP->horizScrollRectOffset = 0;
        tempSpriteWorldP->vertScrollRectOffset = 0;
        tempSpriteWorldP->horizScrollDelta = 0;
        tempSpriteWorldP->vertScrollDelta = 0;
        tempSpriteWorldP->worldMoveProc = 0;
        tempSpriteWorldP->thereAreNonScrollingLayers = 0;
        tempSpriteWorldP->fastBlit = 1;

        tempSpriteWorldP->visScrollRect = windowFrameP->frameRect;

        OffsetRect(&tempSpriteWorldP->visScrollRect,
                   -tempSpriteWorldP->visScrollRect.left,
                   -tempSpriteWorldP->visScrollRect.top);

        tempSpriteWorldP->offscreenScrollRect = tempSpriteWorldP->visScrollRect;

        tempSpriteWorldP->oldVisScrollRect = tempSpriteWorldP->visScrollRect;

        tempSpriteWorldP->tileLayerArray = 0;
        tempSpriteWorldP->tilingCache = 0;
        tempSpriteWorldP->changedTiles = 0;
        tempSpriteWorldP->tilingIsInitialized = 0;
        tempSpriteWorldP->tilingIsOn = 0;
        tempSpriteWorldP->numTilesChanged = 0;
        tempSpriteWorldP->tileChangeProc = 0;
        tempSpriteWorldP->tileMaskDrawProc = SWStdSpriteDrawProc;
        tempSpriteWorldP->partialMaskDrawProc = SWStdSpriteDrawProc;

        tempSpriteWorldP->frameHasOccurred = 0;
        tempSpriteWorldP->frameInterval = 0;

        tempSpriteWorldP->lastMicroseconds = 0;
        tempSpriteWorldP->runningTimeCount = 0;

        backgroundRed(tempSpriteWorldP) = 0;
        backgroundGreen(tempSpriteWorldP) = 0;
        backgroundBlue(tempSpriteWorldP) = 0;
        tempSpriteWorldP->backgroundValue = 0;

        SWClearStickyError();

        *spriteWorldP = tempSpriteWorldP;
    }

    return err;
}
