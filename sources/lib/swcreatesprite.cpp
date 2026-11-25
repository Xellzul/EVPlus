/* swcreatesprite.cpp -- SWCreateSprite (win 0x004744a0, donor line 56). Notes: Research/notes/swcreatesprite.md */
#include "burger.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x004744a0
// MAC: _SWCreateSprite i386 0x000a8b60 ppc 0x000b3b4c
short SWCreateSprite(SpritePtr *newSpriteP, void *spriteStorageP, short maxFrames)
{
    short err = 0;
    SpritePtr tempSpriteP;
    int32_t frameArraySize;

    *newSpriteP = 0;

    tempSpriteP = (spriteStorageP == 0) ? (SpritePtr)AllocAPointer(sizeof(SpriteRec))
                                        : (SpritePtr)spriteStorageP;

    if (tempSpriteP != 0) {
        frameArraySize = (int32_t)maxFrames * sizeof(FramePtr);

        tempSpriteP->frameArray = (FramePtr *)AllocAPointerClear(frameArraySize);

        if (tempSpriteP->frameArray != 0) {
            tempSpriteP->parentSpriteLayerP = 0;
            tempSpriteP->nextSpriteP = 0;
            tempSpriteP->prevSpriteP = 0;

            tempSpriteP->isVisible = 1;
            tempSpriteP->destFrameRect.left = 0;
            tempSpriteP->destFrameRect.top = 0;
            tempSpriteP->destFrameRect.right = 0;
            tempSpriteP->destFrameRect.bottom = 0;

            tempSpriteP->curFrameP = 0;
            tempSpriteP->numFrames = 0;
            tempSpriteP->maxFrames = maxFrames;
            tempSpriteP->curFrameIndex = 0;

            tempSpriteP->spriteCollideProc = 0;

            tempSpriteP->sharedPictGWorld = 0;
            tempSpriteP->sharedMaskGWorld = 0;

            tempSpriteP->scaledWidth = -1;
            tempSpriteP->scaledHeight = -1;

            tempSpriteP->userData = 0;

            tempSpriteP->translucency = 0;
            tempSpriteP->tintRed = 0;
            tempSpriteP->tintGreen = 0;
            tempSpriteP->tintBlue = 0;

            tempSpriteP->rleDataH = 0;
            tempSpriteP->collisionsDisabled = 0;
            tempSpriteP->ownsRLEData = 0;
            tempSpriteP->isEmptySprite = 0;

            *newSpriteP = tempSpriteP;
        } else {
            err = memFullErr;

            DeallocAPointer(tempSpriteP);
        }
    } else {
        err = memFullErr;
    }

    return err;
}
