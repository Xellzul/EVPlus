/* swcreatespriteworld.cpp -- SWEnterSpriteWorld (SpriteWorld.c's order) and SWCreateSpriteWorld (win 0x00477640, donor SpriteWorld.c line 117). Notes: Research/notes/swcreatespriteworld.md */
#include "burger.h"
#include "spriteworld.h"


// FUNCTION: EVNOVA 0x00477630
// MAC: _SWEnterSpriteWorld i386 0x000b6b30 ppc 0x000c2630
int SWEnterSpriteWorld(void)
{
    SWSetStickyIfError(0);
    return 0;
}

/* The world's rects from its window and back frames. */
void SWSetWorldRects(SpriteWorldRec *spriteWorldP)
{
    spriteWorldP->windRect = spriteWorldP->windowFrameP->frameRect;
    spriteWorldP->backRect = spriteWorldP->backFrameP->frameRect;
    spriteWorldP->originalWindRect = spriteWorldP->windowFrameP->frameRect;
    spriteWorldP->originalBackRect = spriteWorldP->backFrameP->frameRect;
    spriteWorldP->visScrollRect = spriteWorldP->windowFrameP->frameRect;
    OffsetRect(&spriteWorldP->visScrollRect, -spriteWorldP->visScrollRect.left,
               -spriteWorldP->visScrollRect.top);
    spriteWorldP->offscreenScrollRect = spriteWorldP->visScrollRect;
    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;
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
        tempSpriteWorldP->doubleRectDrawProc = 0;
        tempSpriteWorldP->postEraseCallBack = 0;
        tempSpriteWorldP->postDrawCallBack = 0;

        SWSetWorldRects(tempSpriteWorldP);

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

        tempSpriteWorldP->backgroundColor.red = 0;   /* +0xdf on 32-bit */
        tempSpriteWorldP->backgroundColor.green = 0;   /* +0xe1 on 32-bit */
        tempSpriteWorldP->backgroundColor.blue = 0;   /* +0xe3 on 32-bit */
        tempSpriteWorldP->backgroundValue = 0;

        SWClearStickyError();

        *spriteWorldP = tempSpriteWorldP;
    }

    return err;
}
