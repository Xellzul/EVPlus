// COMPILE: -lang c++ -bool off
/* swcreaterledata.c -- SWCreateRLEData (win 0x004760c0, SpriteCompiler.c lines 241-242, donor SpriteCompiler.c line 150). Notes: Research/notes/swcreaterledata.md */
#include "macport.h"
#include "spriteworld.h"
#include "burger.h"




/* 0x00472510 -- the donor's seven-argument BlitPixieRLEParsePixels. */


// FUNCTION: EVNOVA 0x004760c0
// MAC: _SWCreateRLEData i386 0x000aec89 ppc 0x000ba1fc
short SWCreateRLEData(SpritePtr srcSpriteP, short requestedDepth)
{
    short err = noErr;
    short frame;
    FramePtr srcFrameP;
    RLEPixelDataHdl spriteData, oldData;
    void **frameData;
    unsigned short depth;
    short maxWidth, maxHeight;
    Rect *rectP;
    Boolean lockedState;
    uint32_t srcBaseOffset;
    uint32_t spriteDataSize;
    uint32_t frameDataSize;

    if (requestedDepth > 0) {
        depth = requestedDepth;
    } else {
        depth = srcSpriteP->frameArray[0]->frameDepth;
        if (depth != 8 && depth != 16 && depth != 32)
            err = kWrongDepthErr;
    }

    oldData = (RLEPixelDataHdl)srcSpriteP->frameArray[0]->rleDataH;

    maxWidth = 0;
    maxHeight = 0;

    for (frame = 0; ((int)err == noErr) && (frame < srcSpriteP->numFrames); frame++) {
        srcFrameP = srcSpriteP->frameArray[frame];

        rectP = &srcFrameP->frameRect;

        if (rectP->right - rectP->left > maxWidth) {
            maxWidth = rectP->right - rectP->left;
        }
        if (rectP->bottom - rectP->top > maxHeight) {
            maxHeight = rectP->bottom - rectP->top;
        }
    }

    spriteData = (RLEPixelDataHdl)AllocAHandle2(kRLEResourceHeaderSize, 0);
    if ((int)err == noErr && spriteData != NULL) {
        (**spriteData).width = maxWidth;
        (**spriteData).height = maxHeight;
        (**spriteData).depth = depth;
        (**spriteData).palette = 0;
        (**spriteData).frames = srcSpriteP->numFrames;
        (**spriteData).reserved1 = 0;
        (**spriteData).reserved2 = 0;
        (**spriteData).reserved3 = 0;

        for (frame = 0; ((int)err == noErr) && (frame < srcSpriteP->numFrames); frame++) {
            srcFrameP = srcSpriteP->frameArray[frame];
            rectP = &srcFrameP->frameRect;

            if (srcFrameP->maskPort == NULL) {
                err = kWrongMaskErr;
                break;
            }

            lockedState = srcFrameP->isFrameLocked;
            if (lockedState == false)
                SWLockFrame(srcFrameP);

            srcBaseOffset =
                    SW_ROWBYTES(srcFrameP, rectP->top) +
                    SW_PIXELBYTES(srcFrameP, rectP->left);

            if (srcFrameP->frameBaseAddr == NULL)
                SW_ASSERT_FAIL("SpriteCompiler.c", 241, 239);
            if (srcFrameP->maskBaseAddr == NULL)
                SW_ASSERT_FAIL("SpriteCompiler.c", 242, 240);

            err = BlitPixieRLEParsePixels(
                    (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
                    (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
                    srcFrameP->frameRowBytes,
                    SW_PIXELBYTES(srcFrameP, rectP->right - rectP->left),
                    (rectP->bottom - rectP->top),
                    depth,
                    &frameData);

            if ((int)err == noErr && frameData != NULL) {
                srcFrameP->rleDataH = spriteData;
                srcFrameP->rleTokenStart = *spriteData + GetAHandleSize((Handle)spriteData);

                LockAHandle((Handle)frameData);
                frameDataSize = GetAHandleSize((Handle)frameData);
                spriteDataSize = GetAHandleSize((Handle)spriteData);
                ResizeAHandle((Handle)spriteData, frameDataSize + spriteDataSize);
                memmove((char *)*spriteData + spriteDataSize, *frameData,
                        frameDataSize);

                DeallocAHandle(frameData);
            }

            if (lockedState == false)
                SWUnlockFrame(srcFrameP);
        }

        if ((int)err != noErr)
            DeallocAHandle(spriteData);
        else {
            if (oldData != NULL)
                DeallocAHandle(oldData);

            srcSpriteP->rleDataH = (char **)spriteData;
            srcSpriteP->ownsRLEData = true;
        }
    } else {
        if ((int)err == noErr)
            err = MemError();
    }

    SWSetStickyIfError(err);
    return err;
}
