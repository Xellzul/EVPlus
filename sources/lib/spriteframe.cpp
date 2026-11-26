/* spriteframe.cpp -- SpriteWorld's SpriteFrame.c, tu-map object O15: frame creation, disposal and locking. Notes: Research/notes/spriteframe.md */
#include "burger.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00476400
// MAC: _SWCreateFrame i386 0x000ad556 ppc 0x000b8a00
short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
                    short depth, unsigned char createGWorld)
{
    short err = 0;
    CGrafPtr frameGWorld;
    SWFrameRec *tempFrameP;
    int32_t numScanLines;

    if (!(frameRect->top >= 0 && frameRect->left >= 0 &&
            frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 72);

    *newFrameP = 0;

    numScanLines = frameRect->bottom - frameRect->top;

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * (numScanLines + 1)));

    if (tempFrameP != 0) {
        tempFrameP->frameRect.top = frameRect->top;
        tempFrameP->frameRect.left = frameRect->left;
        tempFrameP->frameRect.bottom = frameRect->bottom;
        tempFrameP->frameRect.right = frameRect->right;
        tempFrameP->frameDepth = depth;
        tempFrameP->pixelShift = depth >> 4;
        tempFrameP->numScanLines = numScanLines;
        tempFrameP->collisionInset.left = 0;
        tempFrameP->collisionInset.top = 0;
        tempFrameP->collisionInset.right = 0;
        tempFrameP->collisionInset.bottom = 0;
        tempFrameP->usesCollisionInset = 0;
        tempFrameP->rleDataH = 0;

        if (createGWorld) {
            err = NewGWorld(&frameGWorld, depth, frameRect, theGDH);

            if ((int)err == noErr) {
                tempFrameP->framePort = frameGWorld;
                tempFrameP->framePixHndl = GetCWindowPixMap(frameGWorld);

                SWInitializeFrame(tempFrameP);
            }
        } else {
            tempFrameP->framePort = 0;
            tempFrameP->framePixHndl = 0;
            err = 0;
        }

        if ((int)err == noErr) {
            tempFrameP->hotSpotH = 0;
            tempFrameP->hotSpotV = 0;
            tempFrameP->sharesGWorld = 0;
            tempFrameP->isFrameLocked = 0;

            tempFrameP->rleDataH = 0;
            tempFrameP->rleTokenStart = 0;

            tempFrameP->useCount = 0;

            *newFrameP = tempFrameP;
        }
    } else {
        err = memFullErr;
    }

    if ((int)err != noErr) {
        if (tempFrameP != 0) {
            if (tempFrameP->framePort != 0)
                DisposeGWorld(tempFrameP->framePort);
            DeallocAPointer(tempFrameP);
        }
    }

    return err;
}

// FUNCTION: EVNOVA 0x00476800
// MAC: _SWCreateFrameFromCicnResource i386 0x000add00 ppc 0x000b91f4
short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
                                    SWFrameRec **newFrameP, short iconResID,
                                    unsigned char maskType)
{
    short err;
    CGrafPtr saveGWorld;
    SWFrameRec *tempFrameP;
    CIconHandle cIconH;
    Rect frameRect;

    *newFrameP = 0;
    tempFrameP = 0;

    if (destSpriteWorld == 0)
        SWAssertFail("SpriteFrame.c", 288);

    GetPort(&saveGWorld);

    cIconH = GetCIcon(iconResID);

    if (cIconH != 0) {
        QDGetCIconBounds(cIconH, &frameRect);

        err = SWCreateFrame(GetGDevice(), &tempFrameP, &frameRect,
                destSpriteWorld->pixelDepth, kCreateGWorld);
    } else {
        err = resNotFound;
    }

    if ((int)err == noErr) {
        SetPort(tempFrameP->framePort);
        EraseRect(&frameRect);
        nv_BlitImageOr(&frameRect, cIconH);
    }

    if ((int)err == noErr) {
        if ((maskType & kPixelMask) != 0) {
            err = SWCreateGWorldFromCIconMask(destSpriteWorld,
                    &tempFrameP->maskPort, cIconH);

            if ((int)err == noErr) {
                tempFrameP->maskPixHndl =
                        GetCWindowPixMap(tempFrameP->maskPort);

                SWFixImageGWorld(tempFrameP->framePort, tempFrameP->maskPort,
                        0);
            }
        }
    }

    if (cIconH != 0) {
        DisposeCIcon(cIconH);
    }

    if ((int)err == noErr) {
        *newFrameP = tempFrameP;
    }

    if ((int)err != noErr) {
        if (tempFrameP != 0) {
            SWDisposeFrame(&tempFrameP);
        }
    }

    SetPort(saveGWorld);

    return err;
}


// FUNCTION: EVNOVA 0x004769f0
// MAC: _SWCreateFrameFromGWorldAndRectPartial i386 0x000ad7f1 ppc 0x000b8cc4
short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
        CGrafPtr pictGWorld, CGrafPtr maskGWorld, Rect *frameRect, unsigned char maskType)
{
    short err = 0;
    short depth;
    int32_t numScanLines;
    SWFrameRec *tempFrameP;
    Rect *destRectP;

    if (pictGWorld == 0)
        SWAssertFail("SpriteFrame.c", 631);
    if (!(frameRect->top >= 0 && frameRect->left >= 0 &&
            frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 633);

    tempFrameP = 0;
    *newFrameP = 0;

    numScanLines = frameRect->bottom - frameRect->top;
    depth = GetPixDepth(GetCWindowPixMap(pictGWorld));

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * (numScanLines + 1)));

    if (tempFrameP != 0) {
        tempFrameP->framePort = pictGWorld;
        tempFrameP->framePixHndl = GetCWindowPixMap(pictGWorld);
        tempFrameP->frameDepth = depth;

        if ((maskType & kPixelMask) != 0) {
            if (maskGWorld == 0)
                SWAssertFail("SpriteFrame.c", 654);
            tempFrameP->maskPort = maskGWorld;
            tempFrameP->maskPixHndl = GetCWindowPixMap(maskGWorld);
        }

        destRectP = &tempFrameP->frameRect;
        destRectP->top = frameRect->top;
        destRectP->left = frameRect->left;
        destRectP->bottom = frameRect->bottom;
        destRectP->right = frameRect->right;
        tempFrameP->sharesGWorld = 1;
        tempFrameP->numScanLines = numScanLines;

        tempFrameP->useCount = 0;

        SWInitializeFrame(tempFrameP);
    } else {
        err = memFullErr;
    }

    if ((int)err == noErr) {
        if ((int)err == noErr) {
            *newFrameP = tempFrameP;
        } else {
            if (tempFrameP != 0) {
                SWDisposeFrame(&tempFrameP);
            }
        }
    }

    return err;
}

void (*gSWFrameGone)(SWFrameRec *frameP);

/* a frame record freed outright, by a caller that manages its sprite's frames itself */
void SWFreeFrameRecord(SWFrameRec *frameP)
{
    if (frameP == 0)
        return;
    if (gSWFrameGone != 0)
        (*gSWFrameGone)(frameP);
    DeallocAPointer(frameP);
}

/* useCount is unsigned -- `cmp word ptr [esi+0x58], 1' followed by `ja', not `jg'. */

// FUNCTION: EVNOVA 0x00476bd0
// MAC: _SWDisposeFrame i386 0x000ad0d7 ppc 0x000b8494
Boolean SWDisposeFrame(SWFrameRec **oldFramePP)
{
    Boolean frameDisposed;
    SWFrameRec *oldFrameP = *oldFramePP;

    frameDisposed = 0;
    if (oldFrameP != 0) {
        if (oldFrameP->useCount > 1) {
            oldFrameP->useCount--;
        } else {
            frameDisposed = 1;
            if (gSWFrameGone != 0)
                (*gSWFrameGone)(oldFrameP);

            if (oldFrameP->framePort != 0) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->framePort);
                oldFrameP->framePort = 0;
                oldFrameP->framePixHndl = 0;
            }

            if (oldFrameP->maskPort != 0) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->maskPort);
                oldFrameP->maskPort = 0;
                oldFrameP->maskPixHndl = 0;
            }

            DeallocAPointer(oldFrameP);
            *oldFramePP = 0;
        }
    }

    return frameDisposed;
}

// FUNCTION: EVNOVA 0x00476d10
// MAC: _SWInitializeFrame i386 0x000ad25d ppc 0x000b8678
void SWInitializeFrame(SWFrameRec *tempFrameP)
{
    int32_t curScanLine;
    int32_t rowOffset;
    short depth;
    PixMapPtr pixmap;

    if (tempFrameP == NULL)
        SW_ASSERT_FAIL("SpriteFrame.c", 920, 891);
    if (tempFrameP->framePixHndl == NULL) {
        if (tempFrameP->framePort != NULL)
            tempFrameP->framePixHndl = GetCWindowPixMap(tempFrameP->framePort);
    }
    if (tempFrameP->framePixHndl == NULL)
        SW_ASSERT_FAIL("SpriteFrame.c", 928, 899);
    if (tempFrameP->frameDepth == 0)
        SW_ASSERT_FAIL("SpriteFrame.c", 929, 900);

    pixmap = tempFrameP->framePixHndl;
    depth = tempFrameP->frameDepth;

    tempFrameP->frameRowBytes = GetPixRowBytes(pixmap);

    tempFrameP->pixelShift = depth >> 4;

    tempFrameP->scanLinePtrArray = (uint32_t *)(tempFrameP + 1);

    rowOffset = tempFrameP->frameRect.top * tempFrameP->frameRowBytes;
    for (curScanLine = 0; curScanLine < tempFrameP->numScanLines; curScanLine++) {
        tempFrameP->scanLinePtrArray[curScanLine] = rowOffset;
        rowOffset += tempFrameP->frameRowBytes;
    }
}


// FUNCTION: EVNOVA 0x00476e10
// MAC: _SWLockFrame i386 0x000ae467 ppc 0x000b98ec
void SWLockFrame(SWFrameRec *srcFrameP)
{
    PixMapPtr pixMapH;

    if (srcFrameP == 0)
        return;
    if (srcFrameP == 0)
        SWAssertFail("SpriteFrame.c", 1278);

    if (!srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = 1;

        if (srcFrameP->framePort != 0) {
            if (srcFrameP->framePixHndl == 0)
                SWAssertFail("SpriteFrame.c", 1291);

            pixMapH = srcFrameP->framePixHndl;

            srcFrameP->frameBaseAddr = GetPixBaseAddr(pixMapH);
            srcFrameP->frameRowBytes = GetPixRowBytes(pixMapH);
            srcFrameP->frameDepth = GetPixDepth(pixMapH);

        }

        if (srcFrameP->maskPort != 0) {
            if (srcFrameP->maskPixHndl == 0)
                SWAssertFail("SpriteFrame.c", 1348);

            pixMapH = srcFrameP->maskPixHndl;

            srcFrameP->maskBaseAddr = GetPixBaseAddr(pixMapH);
        }
    }
}

// FUNCTION: EVNOVA 0x00476f10
// MAC: _SWUnlockFrame i386 0x000ae5b1 ppc 0x000b9a6c
void SWUnlockFrame(SWFrameRec *srcFrameP)
{
    if (srcFrameP == NULL)
        SW_ASSERT_FAIL("SpriteFrame.c", 1389, 1345);

    if (srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = false;

        srcFrameP->frameBaseAddr = NULL;
        srcFrameP->maskBaseAddr = NULL;
    }
}
