/* spriteworld.h -- the SpriteWorld fork, defined in sources/lib/. Notes:
   Research/declaration-mismatches.md */
#ifndef SPRITEWORLD_H
#define SPRITEWORLD_H
#include "macport.h"
#include "pixel.h"

/* a frame's mask kinds: an enum (an int) on mac, a byte on the win port */
typedef unsigned char MaskType;
#define kPixelMask              1
#define kCreateGWorld           1
#define kWrongDepthErr          105
#define kWrongMaskErr           106


/* --- sprite world --- */
extern short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP,
        CWindowPtr srcWindowP, Rect *worldRectP, Rect *offscreenRectP,
        short maxDepth);                                 /* 0x00477910 */
/* a world's frames made anew for its window port's new size; the world stays locked */
extern short SWResizeSpriteWorld(SpriteWorldPtr spriteWorldP, CWindowPtr srcWindowP,
        Rect *worldRectP, Rect *offscreenRectP);
extern void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP);  /* 0x00478480 */
extern void SWProcessSpriteWorld(SpriteWorldRec *spriteWorldP);  /* 0x004781f0 */
extern void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP,
                                Boolean updateWindow);      /* 0x00477fa0 */
extern void SWLockSpriteWorld(SpriteWorldPtr spriteWorld);   /* 0x00477ba0 */
extern void SWSetPortToBackground(SpriteWorldPtr spriteWorld); /* 0x00477c10 */
extern Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP); /* 0x00478e20 */

/* --- layers --- */
extern short SWCreateSpriteLayer(SpriteLayerRec **spriteLayerP);  /* 0x004770d0 */
extern void SWDisposeSpriteLayer(SpriteLayerRec **spriteLayerPP); /* 0x00477120 */
extern void SWLockSpriteLayer(SpriteLayerRec *spriteLayerP);   /* 0x00477250 */
extern void SWUnlockSpriteLayer(SpriteLayerRec *spriteLayerP); /* 0x00477290 */
extern void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP,
                          SpriteLayerRec *newSpriteLayerP);              /* 0x00477b00 */
extern void SWRemoveSpriteLayer(SpriteWorldRec *spriteWorldP,
                          SpriteLayerRec *oldSpriteLayerP);              /* 0x00477b50 */
extern void SWCollideSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *srcSpriteLayerP,
                          SpriteLayerRec *dstSpriteLayerP);              /* 0x004772d0 */
extern void SWSetLayerCallBack(SpriteLayerRec *spriteLayerP,
        WorldProcPtr callBack);                          /* 0x00477240 */

/* --- frames --- */
extern short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
        short depth, unsigned char createGWorld);        /* 0x00476400 */
extern Boolean SWDisposeFrame(SWFrameRec **oldFramePP);     /* 0x00476bd0 */
extern void SWDisposeWindowFrame(SWFrameRec **oldFramePP); /* 0x00476c70 */
extern void SWLockWindowFrame(SWFrameRec *windowFrameP); /* 0x00476f80 */
extern void SWSetPortToFrame(SWFrameRec *dstFrameP);     /* 0x00476cb0 */
extern void SWInitializeFrame(SWFrameRec *tempFrameP);   /* 0x00476d10 */
extern void SWLockFrame(SWFrameRec *srcFrameP);          /* 0x00476e10 */
extern void SWUnlockFrame(SWFrameRec *srcFrameP);        /* 0x00476f10 */
extern void SWInvertGWorld(CGrafPtr gWorldP);            /* 0x004791b0 */
extern void SWInvertRect(CGrafPtr gWorldP, Rect *rectP); /* 0x004791d0 */

/* --- sprites --- */
extern short SWCloneSprite(SpritePtr cloneSpriteP, SpritePtr *newSpriteP,
                           void *spriteStorageP);        /* 0x00475050 */
extern short SWCreateEmptySprite(SpritePtr *newSpriteP,
                                 void *spriteStorageP);  /* 0x00474720 */
extern short SWCreateSpriteFromCicnResource(SpriteWorldPtr destSpriteWorldP, SpritePtr *slot,
                                 void *spriteStorageP, short cIconID, short maxFrames,
                                 MaskType maskType);              /* 0x00474970 */
extern short SWRemoveSprite(SpriteRec *oldSpriteP);      /* 0x004771c0 */
extern void SWUnlockSprite(SpritePtr srcSpriteP);        /* 0x004756c0 */
extern void SWDisposeSprite(SpritePtr *deadSpritePP);    /* 0x00475470 */
extern short SWCreateSprite(SpritePtr *newSpriteP, void *spriteStorageP,
                            short maxFrames);            /* 0x004744a0 */
extern short SWAddFrame(SpritePtr srcSpriteP, FramePtr newFrameP); /* 0x00475740 */
extern short SWAddSprite(SpriteLayerRec *spriteLayerP, SpriteRec *newSpriteP); /* 0x00477140 */
extern short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
        SWFrameRec **newFrameP, short iconResID,
        MaskType maskType);                         /* 0x00476800 */
extern short SWCreateRLEData(SpritePtr srcSpriteP, short depth);   /* 0x004760c0 */
extern void SWSetBackgroundColor(SpriteWorldPtr spriteWorldP,
                          RGBColor *color);                      /* 0x00477d60 */
extern void SWSetSpriteCollideProc(SpritePtr srcSpriteP,
                          CollideProcPtr collideProc);             /* 0x00475940 */
extern short SWCreateSpriteFromRLEResource(SpritePtr *srcSpriteP, void *spriteStorageP,
                          short rleResID, short maxFrames, short mask);  /* 0x00474e90 */
extern short SWCreateSpriteFromSinglePictXY(SpriteWorldRec *destSpriteWorldP,
                          SpritePtr *newSpriteP, void *spriteStorageP, short pictResID,
                          short maskResID, short frameWidth, short frameHeight,
                          short borderWidth, short borderHeight, short maxFrames,
                          MaskType maskType);
extern void SWWindowMoved(SpriteWorldPtr spriteWorldP);            /* 0x00477f10 */
extern short SWCloneFramesFromMaster(SpritePtr dstSpriteP, SpritePtr masterSpriteP); /* 0x00475f70 */
extern void SWSetSpriteFrameRange(SpritePtr srcSpriteP, short firstFrameIndex,
                                  short lastFrameIndex); /* 0x00475a60 */
extern void SWTempSpriteMoveProc(SpritePtr srcSpriteP);  /* 0x00475800 */
extern short SWSetSpriteDrawProc(SpritePtr srcSpriteP,
                                 DrawProcPtr drawProc);  /* 0x00475970 */
extern void SWLockSprite(SpritePtr srcSpriteP);          /* 0x00475550 */
extern Boolean SWRLECollision(SpritePtr srcSpriteP,
                           SpritePtr dstSpriteP);        /* 0x00475c80 */
extern Boolean SWRLECollisionWithPoint(SpritePtr srcSpriteP,
                                    Point *testPoint);   /* 0x00475e20 */
extern Boolean SWRadiusCollision(SpritePtr srcSpriteP,
                              SpritePtr dstSpriteP);     /* 0x00475be0 */
extern Boolean SWIsSpriteInRect(SpritePtr srcSpriteP,
                             Rect *testRect);            /* 0x00475ec0 */
extern void SWStdSpriteDrawProc(FramePtr srcFrameP, FramePtr dstFrameP,
                                Rect *srcRect, Rect *dstRect);  /* 0x004757a0 */

/* --- draw procs --- */
extern void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470ee0 */
extern void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x004703e0 */
extern void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x004711e0 */
extern void BlitPixieFloatingRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect, short level,
                                 int32_t backgroundValue);                /* 0x00470680 */
extern void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470960 */
extern void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470c20 */
extern Boolean BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00471800 */
extern Boolean BlitPixieRLEPointCollisionProc(SpritePtr srcSpriteP, Point *testPoint,
                                 Rect *destRect);                      /* 0x00471920 */
extern void BlitCopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479cf0 */
extern void CopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479c50 */
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 uint32_t color, int32_t srcShade, int32_t dstShade,
                                 uint32_t srcRamp, uint32_t dstRamp);
                                                                       /* 0x004797b0 */
extern void DrawBeam(FramePtr dstFrameP, Point srcPoint, Point dstPoint, uint32_t color,
                     short halfWidth, uint32_t coronaColor, short fadeStep); /* 0x00479fe0 */
extern void DrawLightningBeam(FramePtr frameA, FramePtr frameB, Point srcPoint, Point dstPoint,
                              uint32_t color, short width, short segPercent, short jitter,
                              short shade);                    /* 0x0047a410 */
extern void EraseBeam(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                      short halfWidth, short fadeStep);        /* 0x0047ac50 */
extern void EraseLightningBeam(FramePtr srcFrameP, FramePtr dstFrameP,
                               Point srcPoint, Point dstPoint,
                               short halfWidth, short segPercent, short jitter); /* 0x0047afd0 */
extern void ComputePixelValues(short pixelSize);                       /* 0x00479210 */
extern void MoveParticles(void);                                       /* 0x0047c800 */
extern SpriteRec *gSWCurrentSpriteBeingDrawn;            /* 0x00870da8 */
extern SpriteWorldRec *gSWCurrentSpriteWorld;            /* 0x008706f0 */
extern unsigned short SWMaskedRGBValue;                  /* 0x007d0500 */
extern short gSWStickyError;                             /* 0x007d0504 */

/* --- how BlitPixieRLETranslucentDrawProc draws a sprite, from its translucency and tint --- */
struct SpriteBlend {
    enum class Kind { Plain, AddOver, Translucent, Hidden };
    Kind kind;
    uint32_t alpha;                     /* dst scale, of 32 */
    uint32_t alphaR, alphaG, alphaB;    /* src scale per channel, of 32 */
    uint32_t overR, overG, overB;       /* ORed in per channel, a pixel pair */
    uint32_t overBias;                  /* src scale against biasColor, of 32; 32 and up: no tint */
    uint32_t biasColor;                 /* the tint colour, a pixel pair */

    static SpriteBlend Of(const SpriteRec &sprite);
};

/* --- the rect blitters: whole-rect copies, src first --- */
extern void BlitPixieRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows);                         /* 0x00471cf0 */
extern void BlitPixieFloatingRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows, unsigned short level);   /* 0x00471d30 */
extern void BlitPixieMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, uint32_t srcRowBytes, uint32_t dstRowBytes,
        unsigned short byteWidth, unsigned short numRows);      /* 0x00471a70 */
extern void BlitPixiePartialMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, uint32_t srcRowBytes, uint32_t dstRowBytes,
        unsigned short byteWidth, unsigned short numRows);      /* 0x00471b70 */
extern Boolean BlitPixieRLECollision(unsigned char *srcTokens, unsigned char *dstTokens,
        Rect *srcRect, Rect *dstRect);                   /* 0x00472190 */
extern Boolean BlitPixieRLECollisionWithPoint(unsigned char *rleData, Point *pt,
        Rect *r);                                        /* 0x00472420 */
extern short BlitPixieRLEParsePixels(unsigned char *srcPixels, unsigned char *maskPixels,
        uint32_t rowBytes, unsigned short byteWidth, unsigned short numRows,
        unsigned short depth, void ***frameData);        /* 0x00472510 */

/* --- the RLE blitters: token stream in, one sprite each --- */
extern void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
                         int32_t dstRowBytes);              /* 0x00471e10 */
extern void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                                int32_t dstRowBytes, Rect *srcBlitRect); /* 0x00471e90 */
extern void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, const SpriteBlend &blend);  /* 0x00472900 */
extern void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes);           /* 0x00472b30 */
extern void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect,
                            const SpriteBlend &blend);      /* 0x00472d70 */
extern void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect);    /* 0x004730d0 */

/* --- the pixie blitters: one scanline each, dst first --- */
extern void BlitPixieMemCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                             int32_t byteWidth);            /* 0x00474350 */
extern void BlitPixieMemSet(unsigned char *dstPixels, uint32_t value,
                            int32_t byteWidth);             /* 0x00474410 */
extern void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                 int32_t byteWidth);        /* 0x00473b60 */
extern void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                     int32_t byteWidth, const SpriteBlend &blend); /* 0x004736c0 */
extern int32_t GetMainDevice(void);                         /* 0x004f2a50 */

/* --- translucent blend (mac _Trans16Bit, inlined in the win build) --- */
static inline unsigned short Trans16Bit(uint32_t src, uint32_t dst, uint32_t alpha)
{
    return Pixel::Blend(src, dst, alpha);
}
extern unsigned short SWTransparentRGBValue;             /* 0x007d0502 */

/* --- lines --- */
extern void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                uint32_t color, short srcShade, short dstShade,
                                short srcRamp, short dstRamp);        /* 0x00479590 */
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint,
                               Rect *clipRectP);         /* 0x0047b3c0 */

/* --- errors --- */
/* an SW_ASSERT site: the fork file's name and line, which differ per build (the win port's
   lines, the mac 1.1.1 build machine's path and lines). SW_LINE1010, set per umbrella, takes a
   1.1.1 line to 1.0.10's: Research/notes/spriteworld-mac.md */
#define SW_LINE(n) (n)
#define SW_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
#define EV_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
extern void SWAssertFail(char *filename, int lineNum);   /* 0x004794d0 */
extern void SWSetStickyIfError(short errNum);            /* 0x004794b0 */

/* --- particles --- */
extern void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP); /* 0x0047bb30 */
extern void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP); /* 0x0047c3a0 */
extern int SWEnterSpriteWorld(void);                           /* 0x00477630 */
extern void SetParticleScrollOffset(int h, int v);                  /* 0x0047b940 */

extern void ShipCollisionHandler(SpriteRec *shipSpr, SpriteRec *otherSpr,
                                 Rect *sectRect);        /* 0x004374f0 */
extern void AsteroidCollisionHandler(SpriteRec *roidSpr, SpriteRec *otherSpr,
                                     Rect *sectRect);    /* 0x00436f70 */

extern uint32_t SWGetValueFromColor(SWFrameRec *dstFrameP, RGBColor *color);
                                                         /* 0x004719d0 */
extern short SWCreateWindowFrame(CWindowPtr srcWindowP, SWFrameRec **newFrameP,
                                 Rect *frameRect, short maxHeight); /* 0x004765d0 */
extern short SWCreateSpriteWorld(void *mainGDH, SpriteWorldRec **spriteWorldP,
                                 SWFrameRec *windowFrameP, SWFrameRec *backFrameP,
                                 SWFrameRec *workFrameP, short maxDepth);
                                                         /* 0x00477640 */
extern void SWSetWorldRects(SpriteWorldRec *spriteWorldP);
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID,
                                    Boolean useGetPicture); /* 0x00478f30 */
extern short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP,
                                    void *pictH);        /* 0x00478fa0 */
struct Image_t;
extern short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *maskGWorldP,
                                    struct Image_t *cIconH); /* 0x00479070 */
extern short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld,
                                    CGrafPtr maskGWorld, Rect *rectP); /* 0x00479250 */
extern short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld,
                                    short maxWidth, short frameHeight,
                                    MaskType maskType);   /* 0x004769d0 */
extern void SWCreateFrameFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld);
                                                         /* 0x004769e0 */
extern short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
                                    CGrafPtr pictGWorldP, CGrafPtr maskGWorldP,
                                    CGrafPtr tempMaskGWorldP, Rect *frameRect,
                                    MaskType maskType);   /* 0x004769f0 */
extern void SWCreateRegionFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld);
                                                         /* 0x00478f20 */

extern void SWFindSpritesToBeRemoved(SpriteWorldPtr spriteWorldP); /* 0x00478cf0 */
extern void SWCloseSprite(SpritePtr deadSpriteP);        /* 0x004754a0 */
extern void SWClearStickyError(void);                    /* 0x004794a0 */
extern void SWStdWorldDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                               Rect *srcRect, Rect *dstRect); /* 0x00477de0 */
extern short SWFixImageGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld,
                              Rect *rectP);              /* 0x00479260 */
extern void SWUnlockWindowFrame(SWFrameRec *windowFrameP); /* 0x00477070 */
extern void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect);
                                                         /* 0x00477f30 */

extern void SWCheckIdleSpriteOverlap(SpriteWorldRec *spriteWorldP,
                                     SpritePtr idleSpriteP,
                                     SpritePtr headActiveSpriteP); /* 0x00478bb0 */

/* the mac SpriteWorld's transparent colour (white) and 16-bit alpha-free colour mask,
   used by SWCreatePixelMaskFromGWorld */
extern RGBColor gSWTransparentColor;
#define kAlphaMask16    0x7FFF

#endif /* SPRITEWORLD_H */
