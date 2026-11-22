/* spriteworld.cpp -- SpriteWorld's SpriteWorld.c, tu-map object O17: the sprite world, its update and animation. Notes: Research/notes/spriteworld.md */

#include "render.h"
#include "game.h"

EXTERN_C_BEGIN

/* The window, back and work frames of a world over `srcWindowP': the window frame over `worldRectP' (NULL =
   the whole port), back and work frames of `offscreenRectP''s size (NULL = the window frame's). */
static short CreateWorldFrames(CWindowPtr srcWindowP, Rect *worldRectP, Rect *offscreenRectP,
                               SWFrameRec **windowFramePP, SWFrameRec **backFramePP, SWFrameRec **workFramePP)
{
    short err = noErr;
    SWFrameRec *windowFrameP, *backFrameP, *workFrameP;
    Rect tempRect, windRect;
    void *mainGDH;
    Rect *srcRectP;

    windowFrameP = backFrameP = workFrameP = 0;

    SetPortWindowPort(srcWindowP);

    if (worldRectP == 0)
        srcRectP = GetCWindowPortRect(srcWindowP);
    else
        srcRectP = worldRectP;

    windRect = *srcRectP;

    mainGDH = (void *)(intptr_t)GetMainDevice();

    if (offscreenRectP == 0) {
        tempRect = windRect;
    } else {
        tempRect = *offscreenRectP;
    }

    OffsetRect(&tempRect, -tempRect.left, -tempRect.top);

    err = SWCreateWindowFrame(srcWindowP, &windowFrameP, &windRect, tempRect.bottom);

    if ((int)err == noErr)
        err = SWCreateFrame(mainGDH, &backFrameP, &tempRect, 16, kCreateGWorld);

    if ((int)err == noErr)
        err = SWCreateFrame(mainGDH, &workFrameP, &tempRect, 16, kCreateGWorld);

    if ((int)err != noErr) {
        if (windowFrameP != 0)
            SWDisposeWindowFrame(&windowFrameP);

        if (backFrameP != 0)
            SWDisposeFrame(&backFrameP);

        if (workFrameP != 0)
            SWDisposeFrame(&workFrameP);
    }

    *windowFramePP = windowFrameP;
    *backFramePP = backFrameP;
    *workFramePP = workFrameP;
    return err;
}

// FUNCTION: EVNOVA 0x00477910
// MAC: _SWCreateSpriteWorldFromWindow i386 0x000b6e56 ppc 0x000c2954
short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP, CWindowPtr srcWindowP,
                                    Rect *worldRectP, Rect *offscreenRectP, short maxDepth)
{
    short err;
    SWFrameRec *windowFrameP, *backFrameP, *workFrameP;

    *spriteWorldP = 0;

    err = CreateWorldFrames(srcWindowP, worldRectP, offscreenRectP, &windowFrameP, &backFrameP, &workFrameP);

    if ((int)err == noErr) {
        err = SWCreateSpriteWorld((void *)(intptr_t)GetMainDevice(), spriteWorldP, windowFrameP,
                                  backFrameP, workFrameP, maxDepth);

        if ((int)err != noErr) {
            SWDisposeWindowFrame(&windowFrameP);
            SWDisposeFrame(&backFrameP);
            SWDisposeFrame(&workFrameP);
        }
    }

    SWSetStickyIfError(err);

    return err;
}

/* A locked world's frames made anew over its window port, which changed size, as
   SWCreateSpriteWorldFromWindow makes them; locked again. */
short SWResizeSpriteWorld(SpriteWorldPtr spriteWorldP, CWindowPtr srcWindowP, Rect *worldRectP,
                          Rect *offscreenRectP)
{
    short err;
    SWFrameRec *windowFrameP, *backFrameP, *workFrameP;

    err = CreateWorldFrames(srcWindowP, worldRectP, offscreenRectP, &windowFrameP, &backFrameP, &workFrameP);
    SWSetStickyIfError(err);
    if ((int)err != noErr)
        return err;

    SWDisposeWindowFrame(&spriteWorldP->windowFrameP);
    SWDisposeFrame(&spriteWorldP->backFrameP);
    SWDisposeFrame(&spriteWorldP->workFrameP);
    spriteWorldP->windowFrameP = windowFrameP;
    spriteWorldP->backFrameP = backFrameP;
    spriteWorldP->workFrameP = workFrameP;
    SWSetWorldRects(spriteWorldP);

    SWLockWindowFrame(windowFrameP);
    SWLockFrame(workFrameP);
    SWLockFrame(backFrameP);
    return noErr;
}


// FUNCTION: EVNOVA 0x00477af0
// MAC: _SWExitSpriteWorld i386 0x000b70b2 ppc 0x000c2b8c
void SWExitSpriteWorld(void)
{
}

// FUNCTION: EVNOVA 0x00477b00
// MAC: _SWAddSpriteLayer i386 0x000b71b0 ppc 0x000c2cf4
void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *newSpriteLayerP)
{
    SpriteLayerRec *tailSpriteLayerP = spriteWorldP->tailSpriteLayerP;

    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 459, 444);
    if (tailSpriteLayerP != NULL)
        tailSpriteLayerP->nextSpriteLayerP = newSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = newSpriteLayerP;
    newSpriteLayerP->prevSpriteLayerP = tailSpriteLayerP;
    newSpriteLayerP->nextSpriteLayerP = NULL;
    spriteWorldP->tailSpriteLayerP = newSpriteLayerP;
}

// FUNCTION: EVNOVA 0x00477b50
// MAC: _SWRemoveSpriteLayer i386 0x000b7140 ppc 0x000c2c48
void SWRemoveSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *oldSpriteLayerP)
{
    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 482, 467);
    if (oldSpriteLayerP->nextSpriteLayerP != NULL)
        oldSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    else
        spriteWorldP->tailSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    if (oldSpriteLayerP->prevSpriteLayerP != NULL)
        oldSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
}


// FUNCTION: EVNOVA 0x00477ba0
// MAC: _SWLockSpriteWorld i386 0x000b734d ppc 0x000c2ef4
void SWLockSpriteWorld(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerPtr curSpriteLayerP;

    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 624, 609);

    SWLockWindowFrame(spriteWorldP->windowFrameP);
    SWLockFrame(spriteWorldP->workFrameP);
    SWLockFrame(spriteWorldP->backFrameP);
    if (spriteWorldP->extraBackFrameP != NULL)
        SWLockFrame(spriteWorldP->extraBackFrameP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL) {
        SWLockSpriteLayer(curSpriteLayerP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}


// FUNCTION: EVNOVA 0x00477c10
// MAC: _SWSetPortToBackground i386 0x000b744a ppc 0x000c3014
void SWSetPortToBackground(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 687, 668);
    SWSetPortToFrame(spriteWorld->backFrameP);
}

// FUNCTION: EVNOVA 0x00477c40
// MAC: _SWSetPortToWorkArea i386 0x000b7494 ppc 0x000c3058
void SWSetPortToWorkArea(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL) {
        SW_ASSERT_FAIL("SpriteWorld.c", 700, 681);
    }
    SWSetPortToFrame(spriteWorld->workFrameP);
}

// FUNCTION: EVNOVA 0x00477c70
// MAC: _SWSetPortToWindow i386 0x000b74e4 ppc 0x000c3098
void SWSetPortToWindow(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld != NULL) {
        if (spriteWorld->screenPort != NULL) {
            SetPort(spriteWorld->screenPort);
            return;
        }
        SWSetPortToFrame(spriteWorld->windowFrameP);
        return;
    }
}


// FUNCTION: EVNOVA 0x00477c90
// MAC: _SWSetSpriteWorldScreenDrawProc i386 0x000b7599 ppc 0x000c3170
short SWSetSpriteWorldScreenDrawProc(SpriteWorldPtr spriteWorld, DrawProcPtr proc)
{
    if (spriteWorld == NULL) {
        SWAssertFail("SpriteWorld.c", 0x2fc);
    }
    if (proc == NULL) {
        SWAssertFail("SpriteWorld.c", 0x2fd);
    }
    spriteWorld->screenDrawProc = proc;
    SWSetStickyIfError(0);
    return 0;
}

// FUNCTION: EVNOVA 0x00477ce0
// MAC: _SWSetPostEraseCallBack i386 0x000b76a8 ppc 0x000c32a0
void SWSetPostEraseCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postEraseCallBack = proc;
}

// FUNCTION: EVNOVA 0x00477cf0
// MAC: _SWSetPostDrawCallBack i386 0x000b76b6 ppc 0x000c32a8
void SWSetPostDrawCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postDrawCallBack = proc;
}

// FUNCTION: EVNOVA 0x00477d00
// MAC: _SWSetSpriteWorldMaxFPS i386 0x000b76c4 ppc 0x000c32b0
void SWSetSpriteWorldMaxFPS(SpriteWorldPtr spriteWorldP, short framesPerSec)
{
    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 822, 811);

    if (framesPerSec > 0 && framesPerSec <= 1000) {
        spriteWorldP->frameInterval = 1000/framesPerSec;
    }

    else {
        spriteWorldP->frameInterval = 0;
    }
}

// FUNCTION: EVNOVA 0x00477d60
// MAC: _SWSetBackgroundColor i386 0x000b7737 ppc 0x000c3324
void SWSetBackgroundColor(SpriteWorldPtr spriteWorldP, RGBColor *color)
{
    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 845, 834);
    if (spriteWorldP->workFrameP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 846, 835);

    spriteWorldP->backgroundColor = *color;
    spriteWorldP->backgroundValue = SWGetValueFromColor(spriteWorldP->workFrameP, color);
}


// FUNCTION: EVNOVA 0x00477de0
// MAC: _SWStdWorldDrawProc i386 0x000b788a ppc 0x000c34bc
void SWStdWorldDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                        Rect *srcRect, Rect *dstRect)
{
    Rect srcCopyBitsRect = *srcRect;
    Rect dstCopyBitsRect = *dstRect;

    if (dstCopyBitsRect.top < dstFrameP->frameRect.top) {
        srcCopyBitsRect.top += dstFrameP->frameRect.top - dstCopyBitsRect.top;
        dstCopyBitsRect.top = dstFrameP->frameRect.top;
        if (dstCopyBitsRect.top >= dstCopyBitsRect.bottom)
            return;
    }

    if (dstCopyBitsRect.bottom > dstFrameP->frameRect.bottom) {
        srcCopyBitsRect.bottom -= dstCopyBitsRect.bottom - dstFrameP->frameRect.bottom;
        dstCopyBitsRect.bottom = dstFrameP->frameRect.bottom;
        if (dstCopyBitsRect.bottom <= dstCopyBitsRect.top)
            return;
    }

    if (dstCopyBitsRect.left < dstFrameP->frameRect.left) {
        srcCopyBitsRect.left += dstFrameP->frameRect.left - dstCopyBitsRect.left;
        dstCopyBitsRect.left = dstFrameP->frameRect.left;
        if (dstCopyBitsRect.left >= dstCopyBitsRect.right)
            return;
    }

    if (dstCopyBitsRect.right > dstFrameP->frameRect.right) {
        srcCopyBitsRect.right -= dstCopyBitsRect.right - dstFrameP->frameRect.right;
        dstCopyBitsRect.right = dstFrameP->frameRect.right;
        if (dstCopyBitsRect.right <= dstCopyBitsRect.left)
            return;
    }

    CopyBits((BitMapPtr)srcFrameP->framePix,(BitMapPtr)dstFrameP->framePix,
             &srcCopyBitsRect, &dstCopyBitsRect, srcCopy, 0);
}


// FUNCTION: EVNOVA 0x00477f10
// MAC: _SWWindowMoved i386 0x000b8d06 ppc 0x000c4c9c
void SWWindowMoved(SpriteWorldPtr spriteWorldP)
{
    SWWindowFrameMoved(spriteWorldP->windowFrameP, &spriteWorldP->windRect);
}

// FUNCTION: EVNOVA 0x00477f30
// MAC: _SWWindowFrameMoved i386 0x000b7f92 ppc 0x000c3b40
void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect)
{
    CGrafPtr savePort;

    if (windowFrameP == 0)
        SWAssertFail("SpriteWorld.c", 1346);
    if (!windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1347);

    GetPort(&savePort);
    SetPort(windowFrameP->framePort);

    if (windowFrameP->isFrameLocked) {
        SWUnlockWindowFrame(windowFrameP);
        SWLockWindowFrame(windowFrameP);
    }

    SetPort(savePort);
}


// FUNCTION: EVNOVA 0x00477fa0
// MAC: _SWUpdateSpriteWorld i386 0x000b9062 ppc 0x000c4ff0
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP, Boolean updateWindow)
{
    UpdateRectStructPtr curRectStructP, nextRectStructP;
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;
    CGrafPtr holdGWorld;
    void *holdGDH;
    short curTileLayer;
    DrawProcPtr saveScreenDrawProc;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1440);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1441);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1442);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1443);

    saveScreenDrawProc = spriteWorldP->screenDrawProc;
    SWSetSpriteWorldScreenDrawProc(spriteWorldP, BlitPixieRectDrawProc);

    GetPort(&holdGWorld);
    holdGDH = GetGWorldDevice(holdGWorld);

    gSWCurrentSpriteWorld = spriteWorldP;
    SWIsWindowInForeground(spriteWorldP);

    SWSetPortToFrame(spriteWorldP->workFrameP);
    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                                       spriteWorldP->workFrameP,
                                       &spriteWorldP->backFrameP->frameRect,
                                       &spriteWorldP->workFrameP->frameRect);

    if (spriteWorldP->postEraseCallBack != 0)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0) {
                curSpriteP->tileDepth = curTileLayer;

                if (curSpriteP->isVisible && curSpriteP->curFrameP->isFrameLocked) {
                    gSWCurrentSpriteBeingDrawn = curSpriteP;

                    (*curSpriteP->frameDrawProc)(curSpriteP->curFrameP,
                                                 spriteWorldP->workFrameP,
                                                 &curSpriteP->curFrameP->frameRect,
                                                 &curSpriteP->destFrameRect);

                    gSWCurrentSpriteBeingDrawn = 0;
                }

                curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;

                curSpriteP->oldFrameRect = curSpriteP->destFrameRect;

                curSpriteP->needsToBeDrawn = 0;
                curSpriteP->needsToBeErased = 0;
                curSpriteP->onTop = 0;
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        if (curSpriteLayerP->layerCallBack != 0)
            (*curSpriteLayerP->layerCallBack)(spriteWorldP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (spriteWorldP->postDrawCallBack != 0)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    if (updateWindow) {
        SWSetPortToFrame(spriteWorldP->windowFrameP);

        (*spriteWorldP->screenDrawProc)(spriteWorldP->workFrameP,
                                        spriteWorldP->windowFrameP,
                                        &spriteWorldP->visScrollRect,
                                        &spriteWorldP->windowFrameP->frameRect);
    }

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while (nextRectStructP != 0) {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DeallocAPointer(curRectStructP);
    }
    spriteWorldP->headUpdateRectP = 0;

    spriteWorldP->numTilesChanged = 0;

    SWSetSpriteWorldScreenDrawProc(spriteWorldP, saveScreenDrawProc);

    gSWCurrentSpriteWorld = 0;

    SetPort(holdGWorld);
}


// FUNCTION: EVNOVA 0x004781f0
// MAC: _SWProcessSpriteWorld i386 0x000b8957 ppc 0x000c485c
void SWProcessSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    unsigned long long curMicroseconds;
    uint32_t millisecDifference;
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP, nextSpriteP;
    SWFrameRec *newFrameP;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1618);

    gSWCurrentSpriteWorld = spriteWorldP;

    curMicroseconds = TimerGetMicroseconds();
    millisecDifference = (curMicroseconds - spriteWorldP->lastMicroseconds) / 1000;

    if (millisecDifference > 0) {
        spriteWorldP->runningTimeCount += millisecDifference;
        // BUG (all versions): the sub-millisecond remainder is dropped on every update, so
        // runningTimeCount (sprite frame/move timing) runs slow -- up to ~1 ms per call
        spriteWorldP->lastMicroseconds = curMicroseconds;
    }

    if (spriteWorldP->runningTimeCount - spriteWorldP->timeOfLastFrame >=
            spriteWorldP->frameInterval) {
        spriteWorldP->frameHasOccurred = 1;
        spriteWorldP->timeOfLastFrame = spriteWorldP->runningTimeCount;
    } else {
        spriteWorldP->frameHasOccurred = 0;
        return;
    }

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWFindSpritesToBeRemoved(spriteWorldP);

    spriteWorldP->thereAreNonScrollingLayers = 0;

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        if (curSpriteLayerP->layerIsNonScrolling) {
            spriteWorldP->thereAreNonScrollingLayers = 1;
        } else if (!curSpriteLayerP->isPaused) {
            curSpriteP = curSpriteLayerP->headSpriteP;

            while (curSpriteP != 0) {
                nextSpriteP = curSpriteP->nextSpriteP;

                if (curSpriteP->curFrameP != 0) {
                    if (!curSpriteP->curFrameP->isFrameLocked)
                        SWLockFrame(curSpriteP->curFrameP);

                    if (curSpriteP->frameTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastFrameChange) >=
                            curSpriteP->frameTimeInterval &&
                        curSpriteP->curFrameP->isFrameLocked)
                    {
                        int32_t currentFrameIndex;

                        curSpriteP->timeOfLastFrameChange = spriteWorldP->runningTimeCount;

                        currentFrameIndex  = curSpriteP->curFrameIndex;
                        currentFrameIndex += curSpriteP->frameAdvance;

                        if (currentFrameIndex < curSpriteP->firstFrameIndex) {
                            if (curSpriteP->frameAdvanceMode == kSWWrapAroundMode) {
                                currentFrameIndex = curSpriteP->lastFrameIndex;
                            } else {
                                curSpriteP->frameAdvance = -curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                            }
                        } else if (currentFrameIndex > curSpriteP->lastFrameIndex) {
                            if (curSpriteP->frameAdvanceMode == kSWWrapAroundMode) {
                                currentFrameIndex = curSpriteP->firstFrameIndex;
                            } else {
                                curSpriteP->frameAdvance = -curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                            }
                        }

                        curSpriteP->curFrameIndex = currentFrameIndex;

                        if (curSpriteP->frameChangeProc != 0) {
                            newFrameP = curSpriteP->frameArray[currentFrameIndex];

                            (*curSpriteP->frameChangeProc)(curSpriteP, newFrameP,
                                    &curSpriteP->curFrameIndex);

                            if (curSpriteP->curFrameIndex < 0) {
                                curSpriteP->curFrameIndex = 0;
                            } else if (curSpriteP->curFrameIndex >= curSpriteP->maxFrames) {
                                curSpriteP->curFrameIndex = curSpriteP->maxFrames - 1;
                            }
                        }

                        newFrameP = curSpriteP->frameArray[curSpriteP->curFrameIndex];

                        if (curSpriteP->curFrameP != newFrameP)
                            SWSetCurrentFrameIndex(curSpriteP, curSpriteP->curFrameIndex);
                    }

                    if (curSpriteP->moveTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastMove) >=
                            curSpriteP->moveTimeInterval)
                    {
                        curSpriteP->timeOfLastMove = spriteWorldP->runningTimeCount;

                        if (curSpriteP->spriteMoveProc != 0)
                            (*curSpriteP->spriteMoveProc)(curSpriteP);
                    }
                }

                curSpriteP = nextSpriteP;
            }
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    SWIsWindowInForeground(spriteWorldP);

    gSWCurrentSpriteWorld = 0;
}




SpriteWorldRec *gSWCurrentSpriteWorld;                   /* 0x008706f0 */

// FUNCTION: EVNOVA 0x00478480
// MAC: _SWAnimateSpriteWorld i386 0x000b95e6 ppc 0x000c5550
void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;
    SpritePtr headActiveSpriteP = 0;
    SpritePtr headIdleSpriteP = 0;
    SpritePtr curActiveSpriteP = 0;
    SpritePtr curIdleSpriteP = 0;
    Rect screenDestRect;
    Rect oldScreenDestRect;
    short curTileLayer;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1984);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1985);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1986);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1987);

    gSWCurrentSpriteWorld = spriteWorldP;

    if (!spriteWorldP->frameHasOccurred)
        return;

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);

    SWSetPortToFrame(spriteWorldP->workFrameP);

    MyProfileEnter(17);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP) {
                curSpriteP->tileDepth = curTileLayer;

                if (curSpriteP->curFrameP->isFrameLocked) {
                    if ((curSpriteP->needsToBeDrawn && curSpriteP->isVisible) ||
                        curSpriteP->needsToBeErased)
                    {
                        if (headActiveSpriteP == 0)
                            headActiveSpriteP = curSpriteP;

                        if (curActiveSpriteP != 0)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                        curActiveSpriteP = curSpriteP;

                        if (curSpriteP->justBecameVisible) {
                            if (!curSpriteP->needsToBeErased) {
                                curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
                            }

                            curSpriteP->justBecameVisible = 0;
                        }

                        curSpriteP->srD3 = 0;

                        curSpriteP->deltaFrameRect.top =
                            SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                        curSpriteP->deltaFrameRect.left =
                            SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                        curSpriteP->deltaFrameRect.bottom =
                            SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                        curSpriteP->deltaFrameRect.right =
                            SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);
                        {
                            short temp;

                            curSpriteP->deltaFrameRect.left &=
                                    (spriteWorldP->workFrameP->leftAlignFactor);

                            temp = curSpriteP->deltaFrameRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                            if (temp != 0)
                            {
                                curSpriteP->deltaFrameRect.right +=
                                        (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                            }

                            curSpriteP->oldFrameRect.left &=
                                (spriteWorldP->workFrameP->leftAlignFactor);

                            temp = curSpriteP->oldFrameRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                            if (temp != 0)
                            {
                                curSpriteP->oldFrameRect.right +=
                                    (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                            }
                        }

                        if ((curSpriteP->deltaFrameRect.right - curSpriteP->deltaFrameRect.left) *
                            (curSpriteP->deltaFrameRect.bottom - curSpriteP->deltaFrameRect.top) >
                            (curSpriteP->oldFrameRect.right - curSpriteP->oldFrameRect.left) *
                            (curSpriteP->oldFrameRect.bottom - curSpriteP->oldFrameRect.top) * 2)
                        {
                            curSpriteP->srD3 = 1;
                        }

                        if (!curSpriteP->onTop || curSpriteP->needsToBeErased) {
                            if (curSpriteP->oldFrameRect.left <=
                                    spriteWorldP->workFrameP->frameRect.right &&
                                curSpriteP->oldFrameRect.right >=
                                    spriteWorldP->workFrameP->frameRect.left &&
                                curSpriteP->oldFrameRect.bottom >=
                                    spriteWorldP->workFrameP->frameRect.top &&
                                curSpriteP->oldFrameRect.top <=
                                    spriteWorldP->workFrameP->frameRect.bottom)
                            {
                                (*spriteWorldP->offscreenDrawProc)(
                                        spriteWorldP->backFrameP,
                                        spriteWorldP->workFrameP,
                                        &curSpriteP->oldFrameRect,
                                        &curSpriteP->oldFrameRect);
                            }
                        }
                    }
                    else if (curSpriteP->isVisible && !curSpriteP->onTop)
                    {
                        if (headIdleSpriteP == 0)
                            headIdleSpriteP = curSpriteP;

                        if (curIdleSpriteP != 0)
                            curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                        curIdleSpriteP = curSpriteP;
                    }
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    MyProfileExit(17);

    if (curActiveSpriteP != 0)
        curActiveSpriteP->nextActiveSpriteP = 0;

    if (curIdleSpriteP != 0)
        curIdleSpriteP->nextIdleSpriteP = 0;

    if (spriteWorldP->postEraseCallBack != 0)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    MyProfileEnter(18);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->isVisible) {
                if (curSpriteP->needsToBeDrawn) {
                    if (curSpriteP->destFrameRect.left <=
                            spriteWorldP->workFrameP->frameRect.right &&
                        curSpriteP->destFrameRect.right >=
                            spriteWorldP->workFrameP->frameRect.left &&
                        curSpriteP->destFrameRect.bottom >=
                            spriteWorldP->workFrameP->frameRect.top &&
                        curSpriteP->destFrameRect.top <=
                            spriteWorldP->workFrameP->frameRect.bottom)
                    {
                        gSWCurrentSpriteBeingDrawn = curSpriteP;

                        (*curSpriteP->frameDrawProc)(
                                curSpriteP->curFrameP,
                                spriteWorldP->workFrameP,
                                &curSpriteP->curFrameP->frameRect,
                                &curSpriteP->destFrameRect);

                        gSWCurrentSpriteBeingDrawn = 0;
                    }
                }
                else
                {
                    SWCheckIdleSpriteOverlap(spriteWorldP, curSpriteP, headActiveSpriteP);
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        if (curSpriteLayerP->layerCallBack != 0)
            (*curSpriteLayerP->layerCallBack)(spriteWorldP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    MyProfileExit(18);

    if (spriteWorldP->postDrawCallBack != 0)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    MyProfileEnter(19);

    SWSetPortToFrame(spriteWorldP->windowFrameP);

    curSpriteP = headActiveSpriteP;

    if (spriteWorldP->fastBlit) {
        while (curSpriteP != 0) {
            if (curSpriteP->srD3) {
                screenDestRect = curSpriteP->destFrameRect;

                oldScreenDestRect = curSpriteP->oldFrameRect;

                oldScreenDestRect.left += spriteWorldP->windRect.left;
                oldScreenDestRect.right += spriteWorldP->windRect.left;
                oldScreenDestRect.top += spriteWorldP->windRect.top;
                oldScreenDestRect.bottom += spriteWorldP->windRect.top;
            }
            else
            {
                screenDestRect = curSpriteP->deltaFrameRect;
            }

            screenDestRect.left += spriteWorldP->windRect.left;
            screenDestRect.right += spriteWorldP->windRect.left;
            screenDestRect.top += spriteWorldP->windRect.top;
            screenDestRect.bottom += spriteWorldP->windRect.top;

            if (screenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                screenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                screenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                screenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom)
            {
                if (curSpriteP->srD3) {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->destFrameRect,
                            &screenDestRect);
                }
                else
                {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->deltaFrameRect,
                            &screenDestRect);
                }
            }

            if (curSpriteP->srD3) {
                if (oldScreenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                    oldScreenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                    oldScreenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                    oldScreenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom)
                {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->oldFrameRect,
                            &oldScreenDestRect);
                }
            }

            curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;

            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;

            curSpriteP->needsToBeDrawn = 0;
            curSpriteP->needsToBeErased = 0;
            curSpriteP->onTop = 0;

            curSpriteP = curSpriteP->nextActiveSpriteP;
        }
    }

    spriteWorldP->numTilesChanged = 0;

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);

    MyProfileExit(19);

    gSWCurrentSpriteWorld = 0;
}

/* The srcSectRect copy goes through a POINTER local -- `mov edi,[ebx+0x50] / add
   edi,0x22' then four movzx/mov word pairs. SW_MIN/SW_MAX stay macros: they
   double-evaluate, and the exe re-reads deltaFrameRect.left and .bottom for the pair. */

// FUNCTION: EVNOVA 0x00478bb0
// MAC: _SWCheckIdleSpriteOverlap i386 0x000b82a0 ppc 0x000c3efc
void SWCheckIdleSpriteOverlap(SpriteWorldRec *spriteWorldP, SpritePtr idleSpriteP,
                              SpritePtr headActiveSpriteP)
{
    SpritePtr activeSpriteP = headActiveSpriteP;
    Rect srcSectRect, dstSectRect;
    Rect *frameRectP;

    while (activeSpriteP != 0) {
        if ((idleSpriteP->oldFrameRect.top < activeSpriteP->deltaFrameRect.bottom) &&
            (idleSpriteP->oldFrameRect.bottom > activeSpriteP->deltaFrameRect.top) &&
            (idleSpriteP->oldFrameRect.left < activeSpriteP->deltaFrameRect.right) &&
            (idleSpriteP->oldFrameRect.right > activeSpriteP->deltaFrameRect.left))
        {
            dstSectRect.left =
                SW_MAX(idleSpriteP->destFrameRect.left, activeSpriteP->deltaFrameRect.left);
            dstSectRect.top =
                SW_MAX(idleSpriteP->destFrameRect.top, activeSpriteP->deltaFrameRect.top);
            dstSectRect.right =
                SW_MIN(idleSpriteP->destFrameRect.right, activeSpriteP->deltaFrameRect.right);
            dstSectRect.bottom =
                SW_MIN(idleSpriteP->destFrameRect.bottom, activeSpriteP->deltaFrameRect.bottom);

            frameRectP = &idleSpriteP->curFrameP->frameRect;
            srcSectRect = *frameRectP;

            srcSectRect.left += (dstSectRect.left - idleSpriteP->destFrameRect.left);
            srcSectRect.top += (dstSectRect.top - idleSpriteP->destFrameRect.top);
            srcSectRect.right -= (idleSpriteP->destFrameRect.right - dstSectRect.right);
            srcSectRect.bottom -= (idleSpriteP->destFrameRect.bottom - dstSectRect.bottom);

            gSWCurrentSpriteBeingDrawn = idleSpriteP;

            (*idleSpriteP->frameDrawProc)(idleSpriteP->curFrameP,
                                          spriteWorldP->workFrameP,
                                          &srcSectRect,
                                          &dstSectRect);

            gSWCurrentSpriteBeingDrawn = 0;
        }

        activeSpriteP = activeSpriteP->nextActiveSpriteP;
    }
}


// FUNCTION: EVNOVA 0x00478cf0
// MAC: _SWFindSpritesToBeRemoved i386 0x000b8042 ppc 0x000c3be8
void SWFindSpritesToBeRemoved(SpriteWorldPtr spriteWorldP)
{
    SpritePtr curSpriteP, nextSpriteP;

    curSpriteP = spriteWorldP->deadSpriteLayerP->headSpriteP;

    while (curSpriteP != NULL) {
        nextSpriteP = curSpriteP->nextSpriteP;

        if (curSpriteP->spriteRemoval == kSWDontRemoveSprite)
            SW_ASSERT_FAIL("SpriteWorld.c", 2613, 2600);

        if (curSpriteP->needsToBeErased == false) {
            SWRemoveSprite(curSpriteP);

            if (curSpriteP->spriteRemoval == kSWRemoveAndDisposeSprite)
                SWDisposeSprite(&curSpriteP);
            else
                curSpriteP->spriteRemoval = kSWDontRemoveSprite;
        }

        curSpriteP = nextSpriteP;
    }
}

// FUNCTION: EVNOVA 0x00478d70
// MAC: _SWFlagRectAsChanged i386 0x000b8758 ppc 0x000c4594
short SWFlagRectAsChanged(SpriteWorldPtr spriteWorldP, Rect *theChangedRect)
{
    UpdateRectStructPtr curRectStructP, newUpdateRectP;
    short temp;
    short err = 0;

    theChangedRect->left &= spriteWorldP->workFrameP->leftAlignFactor;

    temp = theChangedRect->right & spriteWorldP->workFrameP->rightAlignFactor;
    if (temp != 0)
        theChangedRect->right += (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;

    newUpdateRectP = (UpdateRectStructPtr)AllocAPointer(sizeof(UpdateRectStruct));

    if (newUpdateRectP != 0) {
        newUpdateRectP->updateRect = *theChangedRect;
        newUpdateRectP->nextRectStructP = 0;

        if (spriteWorldP->headUpdateRectP == 0) {
            spriteWorldP->headUpdateRectP = newUpdateRectP;
        } else {
            curRectStructP = spriteWorldP->headUpdateRectP;

            while (curRectStructP->nextRectStructP != 0)
                curRectStructP = curRectStructP->nextRectStructP;

            curRectStructP->nextRectStructP = newUpdateRectP;
        }
    } else {
        // BUG: a failed allocation reports success
        err = noErr;
    }

    return err;
}


// FUNCTION: EVNOVA 0x00478e20
// MAC: _SWIsWindowInForeground i386 0x000b88cd ppc 0x000c47c8
Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP)
{
    spriteWorldP->fastBlit = 1;

    return spriteWorldP->fastBlit;
}

// FUNCTION: EVNOVA 0x00478e50
// MAC: _SWHideAllSprites i386 0x000b8c09 ppc 0x000c4ba4
void SWHideAllSprites(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SpriteWorld.c", 2847, 2823);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2848, 2824);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2849, 2825);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2850, 2826);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0 && curSpriteP->isVisible)
                SWSetSpriteVisible(curSpriteP, 0);

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}

EXTERN_C_END
