// COMPILE: -lang c++ -bool off
/* sprite.c -- SpriteWorld's Sprite.c, tu-map object O14: sprite creation, frames, movement and collision. Notes: Research/notes/sprite.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00474970
// MAC: _SWCreateSpriteFromCicnResource i386 0x000aa808 ppc 0x000b55b0
short SWCreateSpriteFromCicnResource(SpriteWorldPtr destSpriteWorldP, SpritePtr *newSpriteP,
                                     void *spriteStorageP, short cIconID, short maxFrames,
                                     MaskType maskType)
{
    short err;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;

    if (destSpriteWorldP == 0)
        SW_ASSERT_FAIL("Sprite.c", 366, 350);

    *newSpriteP = 0;

    err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);

    if ((int)err == noErr) {
        for (frame = 0; frame < maxFrames; frame++) {
            err = SWCreateFrameFromCicnResource(destSpriteWorldP, &newFrameP,
                    cIconID + frame, maskType);

            if ((int)err == noErr) {
                err = SWAddFrame(tempSpriteP, newFrameP);
            }

            if ((int)err != noErr) {
                SWDisposeFrame(&newFrameP);

                if (tempSpriteP->numFrames == 0) {
                    DeallocAPointer(tempSpriteP->frameArray);
                    tempSpriteP->frameArray = 0;
                }

                SWDisposeSprite(&tempSpriteP);
                break;
            }
        }

        if ((int)err == noErr) {
            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}


#define kNoMask             0
#define kOutOfRangeErr      107

// FUNCTION: EVNOVA 0x00474ab0
// MAC: _SWCreateSpriteFromSinglePictXY i386 0x000a9bee ppc 0x000b4adc
short SWCreateSpriteFromSinglePictXY(
        SpriteWorldRec *destSpriteWorldP,
        SpritePtr *newSpriteP,
        void *spriteStorageP,
        short pictResID,
        short maskResID,
        short frameWidth,
        short frameHeight,
        short borderWidth,
        short borderHeight,
        short maxFrames,
        unsigned char maskType)
{
    short err;
    PicHandle tempPic;
    CGrafPtr tempPictGWorldP;
    CGrafPtr tempMaskGWorldP;
    CGrafPtr tempTempMaskGWorldP;
    Rect frameRect;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;

    err = 0;

    if (destSpriteWorldP == 0)
        SWAssertFail("Sprite.c", 719);

    tempPictGWorldP = 0;
    tempMaskGWorldP = 0;
    *newSpriteP = 0;

    tempPic = GetPicture(pictResID);
    if (tempPic == 0) {
        err = resNotFound;
    } else {
        QDReleaseResource(tempPic);
    }

    if ((int)err == noErr) {
        err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempPictGWorldP, pictResID, 0);

        if ((int)err == noErr && maskType != kNoMask)
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempMaskGWorldP, maskResID, 0);

        SWInvertGWorld(tempMaskGWorldP);

        if ((int)err == noErr) {
            if (pictResID == maskResID && tempMaskGWorldP != 0) {
                err = SWCreatePixelMaskFromGWorld(tempPictGWorldP, tempMaskGWorldP, 0);
            }
        }

        if (maxFrames == 0) {
            maxFrames = (GetWindowPortRect(tempPictGWorldP)->right / frameWidth) *
                        (GetWindowPortRect(tempPictGWorldP)->bottom / frameHeight);
        }

        if ((int)err == noErr) {
            err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);
        }

        if ((int)err == noErr) {
            err = SWCreateFrameFromGWorldAndRectStart(&tempTempMaskGWorldP, frameWidth,
                    frameHeight, maskType);
        }

        if ((int)err == noErr) {
            frameRect.right = frameWidth;
            frameRect.left = 0;
            frameRect.top = 0;
            frameRect.bottom = frameHeight;

            for (frame = 0; frame < maxFrames; frame++) {
                err = SWCreateFrameFromGWorldAndRectPartial(&newFrameP, tempPictGWorldP,
                        tempMaskGWorldP, tempTempMaskGWorldP, &frameRect, maskType);

                if ((int)err == noErr) {
                    newFrameP->rleDataH = 0;

                    err = SWAddFrame(tempSpriteP, newFrameP);
                    if (frame < maxFrames - 1) {
                        frameRect.left += frameWidth + borderWidth;
                        frameRect.right = frameRect.left + frameWidth;
                        if (frameRect.right >
                                GetWindowPortRect(tempPictGWorldP)->right) {
                            frameRect.top += frameHeight + borderHeight;
                            frameRect.left = 0;
                            frameRect.right = frameWidth;
                            frameRect.bottom = frameRect.top + frameHeight;

                            if (frameRect.bottom >
                                    GetWindowPortRect(tempPictGWorldP)->bottom) {
                                err = kOutOfRangeErr;
                            }
                        }
                    }
                }

                if ((int)err != noErr) {
                    // BUG (likely, all versions): on kOutOfRangeErr the frame was already added to the sprite,
                    // so it is disposed here and again by SWDisposeSprite (double free); also leaks the temp GWorlds
                    SWDisposeFrame(&newFrameP);
                    SWDisposeSprite(&tempSpriteP);
                    break;
                }
            }

            SWCreateFrameFromGWorldAndRectFinish(tempTempMaskGWorldP);
        }

        if ((int)err == noErr) {
            if ((maskType & kPixelMask) == 0) {
                if (tempMaskGWorldP != 0) {
                    DisposeGWorld(tempMaskGWorldP);
                    tempMaskGWorldP = 0;
                }
            }

            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            tempSpriteP->sharedPictGWorld = tempPictGWorldP;
            tempSpriteP->sharedMaskGWorld = tempMaskGWorldP;

            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}

#define kSolidMask      4


// FUNCTION: EVNOVA 0x00474e90
// MAC: _SWCreateSpriteFromRLEResource i386 0x000a9990 ppc 0x000b48b8
short SWCreateSpriteFromRLEResource(SpritePtr *srcSpriteP, void *spriteStorageP,
                                    short rleResID, short maxFrames, short mask)
{
    SpritePtr tempSpriteP;
    FramePtr tempFrameP;
    RLEPixelDataHdl spriteData;
    void *device;
    short numFrames;
    unsigned char createGWorld;
    short frame;
    short err = 0;
    short depth;
    Rect frameRect;

    device = GetGDevice();

    spriteData = (RLEPixelDataHdl)GetResource(kRLEDResType, rleResID);

    if (spriteData != 0) {
        *srcSpriteP = 0;

        numFrames = (**spriteData).frames;
        depth = (**spriteData).depth;
        SetRect(&frameRect,
            0, 0,
            (**spriteData).width,
            (**spriteData).height);

        if (maxFrames > 0 && numFrames > maxFrames)
            numFrames = maxFrames;

        err = SWCreateSprite(&tempSpriteP, spriteStorageP, numFrames);

        if ((int)err == noErr) {
            createGWorld = (mask & (kSolidMask | kPixelMask)) != 0;

            for (frame = 0; (int)err == noErr && frame < tempSpriteP->maxFrames; frame++) {
                err = SWCreateFrame(device, &tempFrameP, &frameRect, depth, createGWorld);

                if ((int)err == noErr) {
                    tempFrameP->rleDataH = spriteData;
                    tempFrameP->rleTokenStart = 0;

                    err = SWAddFrame(tempSpriteP, tempFrameP);
                }
            }

            if ((int)err == noErr) {
                SWSetSpriteFrameRange(tempSpriteP, 0, numFrames - 1);
                SWSetCurrentFrameIndex(tempSpriteP, 0);

                *srcSpriteP = tempSpriteP;
            } else {
                SWDisposeSprite(&tempSpriteP);
            }
        }

        tempSpriteP->rleDataH = (Handle)spriteData;
        tempSpriteP->ownsRLEData = 1;

        if ((int)err == noErr)
            DetachResource((Handle)spriteData);
    } else {
        err = resNotFound;
    }

    SWSetStickyIfError(err);
    return err;
}


#define kNilParameterErr        104

// FUNCTION: EVNOVA 0x00475050
// MAC: _SWCloneSprite i386 0x000a9419 ppc 0x000b430c
short SWCloneSprite(SpritePtr cloneSpriteP, SpritePtr *newSpriteP, void *spriteStorageP)
{
    SpritePtr tempSpriteP;
    short index;
    int32_t frameArraySize;
    short err = 0;

    if (cloneSpriteP == 0)
        return kNilParameterErr;

    tempSpriteP = (spriteStorageP == 0) ? (SpritePtr)AllocAPointer(sizeof(SpriteRec))
                                        : (SpritePtr)spriteStorageP;
    if (tempSpriteP == 0)
        err = memFullErr;

    if ((int)err == noErr) {
        if (GetAPointerSize(tempSpriteP) < sizeof(SpriteRec))
            SWAssertFail("Sprite.c", 1400);

        *tempSpriteP = *cloneSpriteP;

        tempSpriteP->parentSpriteLayerP = 0;
        tempSpriteP->nextSpriteP = 0;
        tempSpriteP->prevSpriteP = 0;

        tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
        tempSpriteP->realSpriteMoveProc = cloneSpriteP->realSpriteMoveProc;
        tempSpriteP->ownsRLEData = 0;
    }

    if ((int)err == noErr) {
        frameArraySize = cloneSpriteP->maxFrames * sizeof(FramePtr);
        tempSpriteP->frameArray = (FramePtr *)AllocAPointerClear(frameArraySize);

        if (tempSpriteP->frameArray != 0) {
            for (index = 0; index < cloneSpriteP->maxFrames; index++) {
                tempSpriteP->frameArray[index] = cloneSpriteP->frameArray[index];
                if (tempSpriteP->frameArray[index] == 0)
                    SWAssertFail("Sprite.c", 1430);
                tempSpriteP->frameArray[index]->useCount++;
            }
        } else {
            err = memFullErr;
            DeallocAPointer(tempSpriteP);
        }
    }

    if ((int)err == noErr)
        *newSpriteP = tempSpriteP;

    SWSetStickyIfError(err);
    return err;
}


/* +0xd2 marks a sprite that owns none of the things SWCloseSprite frees; +0xd1 is
   RLE-handle OWNERSHIP, since SWCloneSprite copies the handle and clears it. */

// FUNCTION: EVNOVA 0x00475470
// MAC: _SWDisposeSprite i386 0x000a974e ppc 0x000b468c
void SWDisposeSprite(SpritePtr *deadSpritePP)
{
    SpritePtr deadSpriteP = *deadSpritePP;

    if (deadSpriteP != 0) {
        if (!deadSpriteP->isEmptySprite)
            SWCloseSprite(deadSpriteP);

        DeallocAPointer(deadSpriteP);

        *deadSpritePP = 0;
    }
}

// FUNCTION: EVNOVA 0x004754a0
// MAC: _SWCloseSprite i386 0x000a9666 ppc 0x000b459c
void SWCloseSprite(SpritePtr deadSpriteP)
{
    short frame;
    Boolean framesDisposed;

    if (deadSpriteP != 0) {
        if (deadSpriteP->ownsRLEData && deadSpriteP->rleDataH != 0) {
            DeallocAHandle(deadSpriteP->rleDataH);
            deadSpriteP->rleDataH = 0;
        }

        for (frame = 0; frame < deadSpriteP->numFrames; frame++) {
            framesDisposed = SWDisposeFrame(&deadSpriteP->frameArray[frame]);
        }

        /* BUG: read uninitialised when numFrames is 0 */
        if (framesDisposed) {
            if (deadSpriteP->sharedPictGWorld != 0) {
                DisposeGWorld(deadSpriteP->sharedPictGWorld);
                deadSpriteP->sharedPictGWorld = 0;
            }

            if (deadSpriteP->sharedMaskGWorld != 0) {
                DisposeGWorld(deadSpriteP->sharedMaskGWorld);
                deadSpriteP->sharedMaskGWorld = 0;
            }
        }

        if (deadSpriteP->frameArray != 0) {
            DeallocAPointer(deadSpriteP->frameArray);
            deadSpriteP->frameArray = 0;
        }
    }
}

/* Token constants read off the exe: `shr edx,0x18' gives kTokenShift 24 and makes
   TokenDataType unsigned, kEndShapeToken is 0, kLineStartToken 1, kCountMask 0xffffff and
   kRLEResourceHeaderSize 0x10. `dataLength' is SIGNED -- the difference is tested `jle'. */


typedef uint32_t TokenDataType;

#define kTokenShift             24
#define kCountMask              0x00ffffff

// FUNCTION: EVNOVA 0x00475550
// MAC: _SWLockSprite i386 0x000aa965 ppc 0x000b5710
void SWLockSprite(SpritePtr srcSpriteP)
{
    int32_t curFrame;
    FramePtr srcFrameP;
    Handle spriteData;
    int32_t dataLength;
    TokenDataType *tokens;
    int32_t offset;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 1623, 1585);

    for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++)
        SWLockFrame(srcSpriteP->frameArray[curFrame]);

    spriteData = srcSpriteP->rleDataH;
    if (spriteData != 0) {
        LockAHandle(spriteData);

        dataLength = GetAHandleSize(spriteData) - kRLEResourceHeaderSize;
        if (dataLength <= 0)
            SW_ASSERT_FAIL("Sprite.c", 1642, 1604);

        offset = 0;
        tokens = (TokenDataType *)(*spriteData + kRLEResourceHeaderSize);

        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];
            if (srcFrameP->rleDataH != spriteData)
                SW_ASSERT_FAIL("Sprite.c", 1656, 1618);

            srcFrameP->rleTokenStart = tokens;

            while (*tokens != (kEndShapeToken << kTokenShift)) {
                if ((*tokens >> kTokenShift) != kLineStartToken)
                    SW_ASSERT_FAIL("Sprite.c", 1665, 1627);

                offset += (*tokens & kCountMask) + sizeof(TokenDataType);
                tokens = (TokenDataType *)((char *)tokens + sizeof(TokenDataType) +
                                           (*tokens & kCountMask));

                if (offset > dataLength)
                    SW_ASSERT_FAIL("Sprite.c", 1672, 1634);
            }

            offset += sizeof(TokenDataType);
            tokens = (TokenDataType *)((char *)tokens + sizeof(TokenDataType));
        }
    }
}

// FUNCTION: EVNOVA 0x004756c0
// MAC: _SWUnlockSprite i386 0x000aad04 ppc 0x000b5aa8
void SWUnlockSprite(SpritePtr srcSpriteP)
{
    int32_t curFrame;
    Handle spriteData;
    FramePtr srcFrameP;

    if (srcSpriteP == 0)
        SWAssertFail("Sprite.c", 1693);

    if (srcSpriteP->frameArray != 0) {
        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            SWUnlockFrame(srcSpriteP->frameArray[curFrame]);
        }
    }

    spriteData = (Handle)srcSpriteP->frameArray[0]->rleDataH;
    if (spriteData != 0) {
        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];

            srcFrameP->rleTokenStart = 0;
        }

        UnlockAHandle(spriteData);
    }
}


/* kMaxFramesErr = 101, read straight off the exe's `mov ebx, 0x65'.
   SpriteRec layout: macport.h; its evidence: Research/notes/swsetcurrentframeindex.md. */

#define kMaxFramesErr   101

// FUNCTION: EVNOVA 0x00475740
// MAC: _SWAddFrame i386 0x000a9043 ppc 0x000b3eec
short SWAddFrame(SpritePtr srcSpriteP, FramePtr newFrameP)
{
    short err = 0;

    if (srcSpriteP == 0 || newFrameP == 0)
        SW_ASSERT_FAIL("Sprite.c", 1783, 1742);

    if (srcSpriteP->numFrames < srcSpriteP->maxFrames) {
        srcSpriteP->frameArray[srcSpriteP->numFrames] = newFrameP;

        srcSpriteP->numFrames++;
        newFrameP->useCount++;
    } else {
        err = kMaxFramesErr;
    }

    SWSetStickyIfError(err);
    return err;
}


/* maskRgn is never dereferenced -- this port has no Region Manager, so the donor's
   OffsetRgn block is gone -- but the handle is still PASSED to CopyBits as the mask,
   which is why the field is read at all. */

// FUNCTION: EVNOVA 0x004757a0
// MAC: _SWStdSpriteDrawProc i386 0x000ab015 ppc 0x000b5e34
void SWStdSpriteDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect, Rect *dstRect)
{
    if (!srcFrameP->isFrameLocked || !dstFrameP->isFrameLocked)
        SWAssertFail("Sprite.c", 1892);

    CopyBits(GetPortBitMapForCopyBits(srcFrameP->framePort),
             GetPortBitMapForCopyBits(dstFrameP->framePort),
             srcRect, dstRect, srcCopy, srcFrameP->maskRgn);
}


/* `[s+0x8c] = [s+0x90]', then a call through +0x90 guarded on the byte +0x15: that is
   what pins spriteMoveProc/realSpriteMoveProc to +0x8c/+0x90 and needsToBeDrawn to +0x15
   for the rest of the file. */

// FUNCTION: EVNOVA 0x00475800
// MAC: _SWTempSpriteMoveProc i386 0x000ab1c9 ppc 0x000b600c
void SWTempSpriteMoveProc(SpritePtr srcSpriteP)
{
    srcSpriteP->spriteMoveProc = srcSpriteP->realSpriteMoveProc;

    if (srcSpriteP->needsToBeDrawn == 0) {
        if (srcSpriteP->realSpriteMoveProc != 0)
            (*srcSpriteP->realSpriteMoveProc)(srcSpriteP);
    }
}


// FUNCTION: EVNOVA 0x00475830
// MAC: _SWSetCurrentFrameIndex i386 0x000a90b6 ppc 0x000b3f88
void SWSetCurrentFrameIndex(SpritePtr srcSpriteP, short frameIndex)
{
    FramePtr newFrameP;
    short horizOffset, vertOffset;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2025, 1980);

    if (frameIndex < 0)
        frameIndex = 0;

    frameIndex = frameIndex % srcSpriteP->numFrames;

    if (frameIndex < srcSpriteP->numFrames) {
        newFrameP = srcSpriteP->frameArray[frameIndex];

        if (srcSpriteP->curFrameP == 0)
            srcSpriteP->curFrameP = newFrameP;

        horizOffset = (srcSpriteP->destFrameRect.left - newFrameP->frameRect.left) +
                    (srcSpriteP->curFrameP->hotSpotH - newFrameP->hotSpotH);
        vertOffset = (srcSpriteP->destFrameRect.top - newFrameP->frameRect.top) +
                    (srcSpriteP->curFrameP->hotSpotV - newFrameP->hotSpotV);

        srcSpriteP->curFrameP = newFrameP;
        srcSpriteP->destFrameRect = newFrameP->frameRect;

        if (srcSpriteP->scaledWidth > 0)
            srcSpriteP->destFrameRect.right = srcSpriteP->destFrameRect.left +
                                                srcSpriteP->scaledWidth;
        if (srcSpriteP->scaledHeight > 0)
            srcSpriteP->destFrameRect.bottom = srcSpriteP->destFrameRect.top +
                                                srcSpriteP->scaledHeight;

        srcSpriteP->destFrameRect.left += horizOffset;
        srcSpriteP->destFrameRect.right += horizOffset;
        srcSpriteP->destFrameRect.top += vertOffset;
        srcSpriteP->destFrameRect.bottom += vertOffset;

        srcSpriteP->curFrameIndex = frameIndex;

        srcSpriteP->needsToBeDrawn = 1;
    }
}

/* The only store is `[sprite+0x94] = arg2', and +0x94 is spriteCollideProc -- pinned by
   SWTempSpriteMoveProc, which moves +0x90 into +0x8c and so fixes
   realSpriteMoveProc/spriteMoveProc, leaving the donor's next member at +0x94. */

// FUNCTION: EVNOVA 0x00475940
// MAC: _SWSetSpriteCollideProc i386 0x000ab28d ppc 0x000b60fc
void SWSetSpriteCollideProc(SpritePtr srcSpriteP, CollideProcPtr collideProc)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2079, 2034);

    srcSpriteP->spriteCollideProc = collideProc;
}


/* The framePort list repeats Mask and PartialMask, and that duplication is the donor's
   verbatim: where the exe writes a sequence out twice, write it out twice. */


#define kSpriteNotCompiledErr   113

// FUNCTION: EVNOVA 0x00475970
// MAC: _SWSetSpriteDrawProc i386 0x000ab327 ppc 0x000b61ac
short SWSetSpriteDrawProc(SpritePtr srcSpriteP, DrawProcPtr drawProc)
{
    short err = 0;
    FramePtr headFrameP = 0;

    if (srcSpriteP == 0 || drawProc == 0)
        SWAssertFail("Sprite.c", 2112);

    headFrameP = srcSpriteP->frameArray[0];
    if (headFrameP != 0) {
        if (headFrameP->frameDepth < 8) {
            if ((drawProc == BlitPixieRectDrawProc) ||
                (drawProc == BlitPixieMaskDrawProc) ||
                (drawProc == BlitPixiePartialMaskDrawProc) ||
                (drawProc == BlitPixieRLEDrawProc)) {
                err = kWrongDepthErr;
            }
        }
    }

    if (headFrameP->framePort == 0) {
        if ((drawProc == BlitPixieRectDrawProc) ||
            (drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc) ||
            (drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc)) {
            err = kWrongMaskErr;
        }
    }

    if (headFrameP->maskPort == 0) {
        if ((drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc)) {
            err = kWrongMaskErr;
        }
    }

    if (drawProc == BlitPixieRLEDrawProc && headFrameP->rleDataH == 0) {
        err = kSpriteNotCompiledErr;
    }

    if ((int)err == noErr)
        srcSpriteP->frameDrawProc = drawProc;

    SWSetStickyIfError(err);
    return err;
}


// FUNCTION: EVNOVA 0x00475a60
// MAC: _SWSetSpriteFrameRange i386 0x000a91f3 ppc 0x000b4108
void SWSetSpriteFrameRange(SpritePtr srcSpriteP, short firstFrameIndex, short lastFrameIndex)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2253, 2195);

    if (firstFrameIndex < 0)
        firstFrameIndex = 0;

    if (lastFrameIndex > srcSpriteP->maxFrames - 1)
        lastFrameIndex = srcSpriteP->maxFrames - 1;

    srcSpriteP->firstFrameIndex = firstFrameIndex;
    srcSpriteP->lastFrameIndex = lastFrameIndex;

    if (srcSpriteP->curFrameIndex < firstFrameIndex)
        SWSetCurrentFrameIndex(srcSpriteP, firstFrameIndex);
    else if (srcSpriteP->curFrameIndex > lastFrameIndex)
        SWSetCurrentFrameIndex(srcSpriteP, lastFrameIndex);
}


// FUNCTION: EVNOVA 0x00475af0
// MAC: _SWSetSpriteLocation i386 0x000ab640 ppc 0x000b656c
void SWSetSpriteLocation(SpritePtr srcSpriteP, short horizLoc, short vertLoc)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2308, 2250);

    horizLoc -= srcSpriteP->curFrameP->hotSpotH;
    vertLoc -= srcSpriteP->curFrameP->hotSpotV;

    srcSpriteP->destFrameRect.bottom = vertLoc + (srcSpriteP->destFrameRect.bottom -
                                                srcSpriteP->destFrameRect.top);
    srcSpriteP->destFrameRect.right = horizLoc + (srcSpriteP->destFrameRect.right -
                                                srcSpriteP->destFrameRect.left);
    srcSpriteP->destFrameRect.top = vertLoc;
    srcSpriteP->destFrameRect.left = horizLoc;

    srcSpriteP->deltaFrameRect = srcSpriteP->destFrameRect;

    srcSpriteP->needsToBeDrawn = 1;
}


// FUNCTION: EVNOVA 0x00475b80
// MAC: _SWSetSpriteVisible i386 0x000a9590 ppc 0x000b44b8
void SWSetSpriteVisible(SpritePtr srcSpriteP, unsigned char isVisible)
{
    if (srcSpriteP == 0)
        return;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2398, 2339);

    if (srcSpriteP->isVisible != isVisible) {
        if (isVisible && !srcSpriteP->needsToBeDrawn && !srcSpriteP->needsToBeErased)
            srcSpriteP->justBecameVisible = 1;

        srcSpriteP->isVisible = isVisible;
        srcSpriteP->needsToBeDrawn = 1;
        srcSpriteP->needsToBeErased = !isVisible;
    }
}


/* `/2' on a signed int is the `cmp 0x80000000 / sbb -1 / sar 1' triple, which is what
   pins the radii SIGNED and not the donor's unsigned-looking widths. */

// FUNCTION: EVNOVA 0x00475be0
// MAC: _SWRadiusCollision i386 0x000abc88 ppc 0x000b6ce4
Boolean SWRadiusCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Point centerOne, centerTwo;
    short radiusOne, radiusTwo;
    short horizDistance, vertDistance;

    if (srcSpriteP == 0 || dstSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2670, 2610);

    radiusOne = (srcSpriteP->destFrameRect.right - srcSpriteP->destFrameRect.left) / 2;
    radiusTwo = (dstSpriteP->destFrameRect.right - dstSpriteP->destFrameRect.left) / 2;

    centerOne.h = srcSpriteP->destFrameRect.left + radiusOne;
    centerOne.v = srcSpriteP->destFrameRect.top + radiusOne;
    centerTwo.h = dstSpriteP->destFrameRect.left + radiusTwo;
    centerTwo.v = dstSpriteP->destFrameRect.top + radiusTwo;

    horizDistance = centerOne.h - centerTwo.h;
    vertDistance = centerOne.v - centerTwo.v;

    return ((horizDistance * horizDistance) + (vertDistance * vertDistance) <
        ((radiusOne + radiusTwo) * (radiusOne + radiusTwo)));
}


/* sectRect never reaches the stack -- mwcc keeps all four members in registers because
   nothing takes its address; only srcSectRect and dstSectRect, which are passed by
   pointer, get frame slots. */



// FUNCTION: EVNOVA 0x00475c80
// MAC: _SWRLECollision i386 0x000abfbd ppc 0x000b6ff8
Boolean SWRLECollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Boolean collision;
    Rect sectRect;
    Rect srcSectRect;
    Rect dstSectRect;
    FramePtr srcFrameP, dstFrameP;

    if (srcSpriteP == 0 || dstSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2761, 2701);
    if (srcSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2762, 2702);
    if (dstSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2763, 2703);
    if (!srcSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2764, 2704);
    if (!dstSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2765, 2705);

    sectRect.left = SW_MAX(srcSpriteP->destFrameRect.left, dstSpriteP->destFrameRect.left);
    sectRect.top = SW_MAX(srcSpriteP->destFrameRect.top, dstSpriteP->destFrameRect.top);
    sectRect.right = SW_MIN(srcSpriteP->destFrameRect.right, dstSpriteP->destFrameRect.right);
    sectRect.bottom = SW_MIN(srcSpriteP->destFrameRect.bottom, dstSpriteP->destFrameRect.bottom);

    if (sectRect.right > sectRect.left && sectRect.bottom > sectRect.top) {
        srcFrameP = srcSpriteP->curFrameP;
        dstFrameP = dstSpriteP->curFrameP;

        srcSectRect.left = (sectRect.left - srcSpriteP->destFrameRect.left);
        srcSectRect.top = (sectRect.top - srcSpriteP->destFrameRect.top);
        srcSectRect.right = (sectRect.right - srcSpriteP->destFrameRect.left);
        srcSectRect.bottom = (sectRect.bottom - srcSpriteP->destFrameRect.top);

        dstSectRect.left = (sectRect.left - dstSpriteP->destFrameRect.left);
        dstSectRect.top = (sectRect.top - dstSpriteP->destFrameRect.top);
        dstSectRect.right = (sectRect.right - dstSpriteP->destFrameRect.left);
        dstSectRect.bottom = (sectRect.bottom - dstSpriteP->destFrameRect.top);

        collision = BlitPixieRLECollisionProc(srcSpriteP, dstSpriteP, &srcSectRect, &dstSectRect);
    } else
        collision = 0;

    return collision;
}

// FUNCTION: EVNOVA 0x00475e20
// MAC: _SWRLECollisionWithPoint i386 0x000ac1ab ppc 0x000b7198
Boolean SWRLECollisionWithPoint(SpritePtr srcSpriteP, Point *testPoint)
{
    Boolean collision;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2810, 2750);
    if (srcSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2811, 2751);
    if (!srcSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2812, 2752);

    if (testPoint->h >= srcSpriteP->destFrameRect.left &&
        testPoint->h <= srcSpriteP->destFrameRect.right &&
        testPoint->v >= srcSpriteP->destFrameRect.top &&
        testPoint->v <= srcSpriteP->destFrameRect.bottom)
    {
        collision = BlitPixieRLEPointCollisionProc(srcSpriteP, testPoint,
                                                   &srcSpriteP->destFrameRect);
    } else
        collision = 0;

    return collision;
}

/* SW_COLLISION_RECT_* is destFrameRect adjusted by the current frame's collisionInset:
   sprite +0x1a/+0x1c/+0x1e/+0x20 against frame +0x2e/+0x30/+0x32/+0x34, which is how the
   inset's four members are pinned. */

// FUNCTION: EVNOVA 0x00475ec0
// MAC: _SWIsSpriteInRect i386 0x000ac282 ppc 0x000b7288
Boolean SWIsSpriteInRect(SpritePtr srcSpriteP, Rect *testRect)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2836, 2776);

    return ((srcSpriteP->destFrameRect.top + srcSpriteP->curFrameP->collisionInset.top)
                < testRect->bottom) &&
           ((srcSpriteP->destFrameRect.bottom - srcSpriteP->curFrameP->collisionInset.bottom)
                > testRect->top) &&
           ((srcSpriteP->destFrameRect.left + srcSpriteP->curFrameP->collisionInset.left)
                < testRect->right) &&
           ((srcSpriteP->destFrameRect.right - srcSpriteP->curFrameP->collisionInset.right)
                > testRect->left);
}

