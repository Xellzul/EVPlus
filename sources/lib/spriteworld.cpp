/* spriteworld.cpp -- SpriteWorld's SpriteWorld.c, tu-map object O17: the sprite world, its update and animation. Notes: Research/notes/spriteworld.md */

#include "render.h"
#include "game.h"

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


// FUNCTION: EVNOVA 0x00477c70
// MAC: _SWSetPortToWindow i386 0x000b74e4 ppc 0x000c3098
void SWSetPortToWindow(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld != NULL && spriteWorld->windowPort != NULL)
        SetPort(spriteWorld->windowPort);
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

/* The game's drawing of the world, called by Animate and Update after their bookkeeping. */
void (*gSWDraw)(SpriteWorldRec *spriteWorldP);

/* a sprite as drawing leaves it: shown where it is now, nothing more to draw or erase */
static void SettleSprite(SpritePtr spriteP)
{
    spriteP->deltaFrameRect = spriteP->destFrameRect;
    spriteP->oldFrameRect = spriteP->destFrameRect;
    spriteP->needsToBeDrawn = 0;
    spriteP->needsToBeErased = 0;
    spriteP->onTop = 0;
}

// FUNCTION: EVNOVA 0x00477fa0
// MAC: _SWUpdateSpriteWorld i386 0x000b9062 ppc 0x000c4ff0
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1440);

    SWIsWindowInForeground(spriteWorldP);
    for (curSpriteLayerP = spriteWorldP->headSpriteLayerP; curSpriteLayerP != 0;
         curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP) {
        for (curSpriteP = curSpriteLayerP->headSpriteP; curSpriteP != 0; curSpriteP = curSpriteP->nextSpriteP) {
            if (curSpriteP->curFrameP != 0)
                SettleSprite(curSpriteP);
        }
    }
    if (gSWDraw != 0)
        (*gSWDraw)(spriteWorldP);
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

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
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

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    SWIsWindowInForeground(spriteWorldP);
}




/* SWAnimateSpriteWorld's bookkeeping for one layer, without its blits: a sprite it would draw or erase is
   settled -- once the frame reaches the window, as fastBlit says */
static void SettleLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *layerP)
{
    for (SpritePtr spriteP = layerP->headSpriteP; spriteP != 0; spriteP = spriteP->nextSpriteP) {
        if (spriteP->curFrameP == 0 || !spriteP->curFrameP->isFrameLocked
                || !((spriteP->needsToBeDrawn && spriteP->isVisible) || spriteP->needsToBeErased))
            continue;
        if (spriteP->justBecameVisible) {
            if (!spriteP->needsToBeErased)
                spriteP->oldFrameRect = spriteP->destFrameRect;
            spriteP->justBecameVisible = 0;
        }
        if (spriteWorldP->fastBlit)
            SettleSprite(spriteP);
    }
}

// FUNCTION: EVNOVA 0x00478480
// MAC: _SWAnimateSpriteWorld i386 0x000b95e6 ppc 0x000c5550
void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1984);

    if (!spriteWorldP->frameHasOccurred)
        return;

    /* every layer settled, the dead sprites' too, then the world drawn */
    for (SpriteLayerRec *layerP = spriteWorldP->headSpriteLayerP; layerP != 0; layerP = layerP->nextSpriteLayerP)
        SettleLayer(spriteWorldP, layerP);
    SettleLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    if (gSWDraw != 0)
        (*gSWDraw)(spriteWorldP);
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
