/* spriteworld.h -- the SpriteWorld fork, defined in sources/lib/. Notes:
   Research/declaration-mismatches.md */
#ifndef SPRITEWORLD_H
#define SPRITEWORLD_H
#include "gametypes.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* a frame's mask kinds: an enum (an int) on mac, a byte on the win port */
#ifdef MACBUILD
typedef enum MaskType {
    kNoMask = 0,
    kPixelMask = 1,
    kRegionMask = 2,
    kSolidMask = 4
} MaskType;
#else
typedef unsigned char MaskType;
#define kPixelMask              1
#endif
#define kCreateGWorld           1
#define kWrongDepthErr          105
#define kWrongMaskErr           106


/* --- sprite world --- */
extern short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP,
        CWindowPtr srcWindowP, Rect *worldRectP, Rect *offscreenRectP,
        short maxDepth);                                 /* 0x00477910 */
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
#if !defined(MACBUILD) || !V111 /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreateRLEData(SpritePtr srcSpriteP, short depth);   /* 0x004760c0 */
#endif
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
extern Boolean sw_depth_ok(SpriteWorldPtr world);                    /* 0x00478f10 */
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
#ifdef MACBUILD
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 uint32_t color, uint32_t alpha, uint32_t dstShade,
                                 uint32_t srcRamp, uint32_t dstRamp);
#else
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 uint32_t color, int32_t srcShade, int32_t dstShade,
                                 uint32_t srcRamp, uint32_t dstRamp);
                                                                       /* 0x004797b0 */
#endif
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
#ifndef MACBUILD
extern void ComputePixelValues(short pixelSize);                       /* 0x00479210 */
#endif
#ifdef MACBUILD
extern short GetGDeviceDepth(GDHandle theGDH);
#else
extern short GetGDeviceDepth(void *theGDH);                            /* 0x0047b7c0 */
#endif
extern void MoveParticles(void);                                       /* 0x0047c800 */
extern SpriteRec *gSWCurrentSpriteBeingDrawn;            /* 0x00870da8 */
extern SpriteWorldRec *gSWCurrentSpriteWorld;            /* 0x008706f0 */
#pragma cplusplus on
EXTERN_CPP_BEGIN
#ifndef MACBUILD
extern unsigned short SWMaskedRGBValue;                  /* 0x007d0500 */
extern short gSWStickyError;                             /* 0x007d0504 */
#endif
EXTERN_CPP_END
#pragma cplusplus reset

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
extern void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes);           /* 0x00472900 */
extern void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes);           /* 0x00472b30 */
extern void BlitPixieRLETranslucent8(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes);           /* 0x00472bb0 */
extern void BlitPixieRLEAddOver8(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes);           /* 0x00472cf0 */
extern void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect);    /* 0x00472d70 */
extern void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect);    /* 0x004730d0 */
extern void BlitPixieRLETranslucentClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect);    /* 0x00473270 */
extern void BlitPixieRLEAddOverClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
                            int32_t dstRowBytes, Rect *srcBlitRect);    /* 0x00473500 */

/* --- the pixie blitters: one scanline each, dst first --- */
#if defined(MAC1010) || !(defined(MACBUILD) && defined(SW_DEFINES_FLOATINGRECT8))   /* BlitPixieRect.o calls it undeclared */
extern void BlitPixieTranslucentClearCopy8(unsigned char *dstPixels,
        unsigned char *srcPixels, int32_t numPixels,
        unsigned char clearValue);                       /* 0x00473dc0 */
#endif
#if defined(MACBUILD) && !defined(MAC1010) && !defined(SW_DEFINES_FLOATINGRECT8)
extern void BlitPixieFloatingRect8();   /* the fork calls it with no prototype in scope */
#else
extern void BlitPixieFloatingRect8(unsigned char *srcPixels,
        unsigned char *dstPixels, uint32_t srcRowBytes, uint32_t dstRowBytes,
        unsigned short byteWidth, unsigned short numRows,
        unsigned short backgroundValue);                 /* 0x00471dc0 */
#endif
extern void BlitPixieMemCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                             int32_t byteWidth);            /* 0x00474350 */
extern void BlitPixieMemSet(unsigned char *dstPixels, uint32_t value,
                            int32_t byteWidth);             /* 0x00474410 */
extern void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                 int32_t byteWidth);        /* 0x00473b60 */
extern void BlitPixieAddOverCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                  int32_t count);           /* 0x00473fa0 */
extern void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                     int32_t byteWidth);    /* 0x004736c0 */
extern void BlitPixieTranslucentCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                      int32_t count);       /* 0x00473c20 */
extern void BlitPixieTranslucentBiasCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                          int32_t count);   /* 0x00473e00 */
extern void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
                         int32_t dstRowBytes);              /* 0x00471e10 */
extern void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                                int32_t dstRowBytes, Rect *srcBlitRect); /* 0x00471e90 */
extern uint32_t fix_power_of_two(uint32_t level);  /* 0x004736a0 */
#ifndef MACBUILD
extern int32_t GetMainDevice(void);                         /* 0x004f2a50 */
#endif

/* --- the blitters' shared state --- */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern uint32_t alpha;                              /* 0x007ceac8 */
extern uint32_t alphaR;                             /* 0x007ceacc */
extern uint32_t alphaG;                             /* 0x007cead0 */
extern uint32_t alphaB;                             /* 0x007cead4 */
extern uint32_t overR;                              /* 0x007cead8 */
extern uint32_t overG;                              /* 0x007ceadc */
extern uint32_t overB;                              /* 0x007ceae0 */
extern uint32_t overBias;                           /* 0x007ceae4 */
extern short scaleR;                                     /* 0x007ceae8 */
extern short scaleG;                                     /* 0x007ceaea */
extern short scaleB;                                     /* 0x007ceaec */
extern uint32_t biasColor;                          /* 0x007ceaf0 */
extern unsigned char *alphaTable[5][256];                /* 0x007ceaf4 */
EXTERN_CPP_END
#pragma cplusplus reset

/* --- translucent blends (mac _Trans8Bit/_Trans16Bit, inlined in the win build) --- */
#if defined(MACBUILD) && (!defined(MAC1010) || defined(SW_DEFINES_TRANS))
#ifndef SW_DEFINES_TRANS
extern char Trans8Bit(uint32_t src, uint32_t dst, int32_t alpha);
extern unsigned short Trans16Bit(uint32_t src, uint32_t dst, int32_t alpha);
#endif
#else
static inline char Trans8Bit(char src, char dst, uint32_t alpha)
{
    /* BUG: any alpha above 3 skips the blend, so (31 - alpha) >> 3 only ever selects row 3 */
    if (alpha > 3)
        return src;
    return alphaTable[(31 - alpha) >> 3][src][dst];
}

static inline unsigned short Trans16Bit(uint32_t src, uint32_t dst, uint32_t alpha)
{
    uint32_t srcSplit, dstSplit, blended;

    srcSplit = SW_SPLIT15(src);
    dstSplit = SW_SPLIT15(dst);
    blended = srcSplit * alpha + dstSplit * (32 - alpha);
    return ((blended >> 5) & 0x7c1f) | ((blended >> 20) & 0x3e0);
}
#endif

#pragma cplusplus on
EXTERN_CPP_BEGIN
#ifndef MACBUILD
extern unsigned short SWTransparentRGBValue;             /* 0x007d0502 */
#endif
EXTERN_CPP_END
#pragma cplusplus reset

/* --- lines --- */
extern void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                uint32_t color, short srcShade, short dstShade,
                                short srcRamp, short dstRamp);        /* 0x00479590 */
#ifdef MACBUILD
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint, Rect *clipRect);
#else
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint,
                               Rect *clipRectP);         /* 0x0047b3c0 */
#endif

/* --- errors --- */
/* an SW_ASSERT site: the fork file's name and line, which differ per build (the win port's
   lines, the mac 1.1.1 build machine's path and lines). SW_LINE1010, set per umbrella, takes a
   1.1.1 line to 1.0.10's: Research/notes/spriteworld-mac.md */
#ifdef MAC1010
#ifndef SW_LINE1010
#define SW_LINE1010(n) (n)
#endif
#define SW_LINE(n) SW_LINE1010(n)
#else
#define SW_LINE(n) (n)
#endif
#ifdef MACBUILD
#ifdef MAC1010
#define kSWSourceDir ""
#else
#define kSWSourceDir "/Users/jerome/Ambrosia/ev_nova1.1.0/sources/3rdparty/ev_nova/__obsolete__/Libraries/SpriteWorld 2.3.1/SpriteWorld Files/Sources/"
#endif
#define SW_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(kSWSourceDir file, SW_LINE(macLine))
/* the fork's Carbon pixmap accessors */
#define SWGetPixBaseAddr(pm) GetPixBaseAddr(pm)
#ifdef MAC1010
#define SWGetPixRowBytes(pm) ((**(pm)).rowBytes & 0x3FFF)
#define SWGetPixBitDepth(pm) ((**(pm)).pixelSize)
#else
#define SWGetPixRowBytes(pm) GetPixRowBytes(pm)
#define SWGetPixBitDepth(pm) GetPixDepth(pm)
#endif
/* the mac bodies' SW_ASSERT, with the line of the mac file named by the umbrella's SW_FILE */
#define SW_ASSERT(condition, macLine) if (!(condition)) SWAssertFail(kSWSourceDir SW_FILE, SW_LINE(macLine))
/* an engine assert site: the 1.1.1 build machine's engine path and line */
#define EV_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(kEngineSourceDir file, SW_LINE(macLine))
#else
#define SW_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
#define EV_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
#endif
extern void SWAssertFail(char *filename, int lineNum);   /* 0x004794d0 */
extern void SWSetStickyIfError(short errNum);            /* 0x004794b0 */

/* --- particles --- */
extern void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP); /* 0x0047bb30 */
extern void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP); /* 0x0047c3a0 */
#ifdef MACBUILD
extern OSErr SWEnterSpriteWorld(void);
#else
extern int SWEnterSpriteWorld(void);                           /* 0x00477630 */
#endif
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
#ifdef MACBUILD
#ifndef IMPLICIT_SWGWORLDS /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID);
#endif
#else
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID,
                                    Boolean useGetPicture); /* 0x00478f30 */
#endif
extern short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP,
                                    void *pictH);        /* 0x00478fa0 */
struct Image_t;
extern short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *maskGWorldP,
                                    struct Image_t *cIconH); /* 0x00479070 */
#ifndef IMPLICIT_SWGWORLDS /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld,
                                    CGrafPtr maskGWorld, Rect *rectP); /* 0x00479250 */
#endif
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

#ifdef MACBUILD
/* the mac 1.1.1 SpriteWorld's functions the win port never had */
extern short SWCreateRegionMaskFromGWorld(CGrafPtr gWorld, Rect *rectP, RgnHandle maskRgn);
extern void SWSetTransparentColor(RGBColor *color);
extern RGBColor gSWTransparentColor;
extern Rect *GetCPortPortRect(CGrafPtr port);
extern short SWCreateMaskAndFixImage(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP);
extern void SWCopyGWorlds(CGrafPtr srcGWorldP, CGrafPtr dstGWorldP);
extern void SWFlipPixelsHorizontally(CGrafPtr dstGWorldP, Rect *rectP);
extern void SWFlipPixelsVertically(CGrafPtr dstGWorldP, Rect *rectP);
extern short SWBlackenGWorld(CGrafPtr oldGWorld);
extern short SWWhitenGWorld(CGrafPtr oldGWorld);
extern short SWFastWhitenGWorld(CGrafPtr pictureGWorld);

/* the masking blitters' alpha-free colour masks */
#define kAlphaMask16    0x7FFF
#define kAlphaMask32    0x00FFFFFFL

/* SpriteWorld's error codes the win port never names */
#define kMaxFramesErr               101
#define kNilParameterErr            104
#define kOutOfRangeErr              107
#define kSpriteNotCompiledErr       113
#define kSpriteAlreadyInLayer       115

/* BlitPixieInterface.c's draw procs the win port dropped */
typedef struct BlitPixieOffsetInfo {
    int32_t srcOffsetAtoB;
    int32_t srcOffsetBtoA;
    int32_t dstOffsetAtoB;
    int32_t dstOffsetBtoA;
} BlitPixieOffsetInfo, *BlitPixieOffsetInfoPtr;
typedef void (*BlitFuncPtr)(int32_t srcRowBytes, int32_t dstRowBytes, char *srcBaseAddr,
        char *dstBaseAddr);
extern void BlitPixieClear(unsigned char *dstPixel, uint32_t dstRowBytes,
        unsigned short bytes, unsigned short rows, uint32_t color);
typedef struct StarArray {
    short horizLoc;
    short vertLoc;
    short oldHorizLoc;
    short oldVertLoc;
    short horizSpeed;
    short vertSpeed;
    uint32_t color;
    Boolean isOn;
    Boolean needsToBeErased;
    short userData;
} StarArray, *StarArrayPtr;
extern void SWGetColorFromValue(FramePtr dstFrameP, RGBColor *color, uint32_t pixelValue);
extern short SWFlagRectAsChanged(SpriteWorldPtr spriteWorldP, Rect *theChangedRect);

/* SpriteLayer.c's functions the win port never had */

/* Sprite.c's types and functions the win port never had */
typedef struct RectArray {
    short numFrames;
    Rect frameRects[];
} RectArray, *RectArrayPtr, **RectArrayHdl;

#define SW_COLLISION_RECT_TOP(spriteP) \
    (spriteP->destFrameRect.top + spriteP->curFrameP->collisionInset.top)
#define SW_COLLISION_RECT_LEFT(spriteP) \
    (spriteP->destFrameRect.left + spriteP->curFrameP->collisionInset.left)
#define SW_COLLISION_RECT_BOTTOM(spriteP) \
    (spriteP->destFrameRect.bottom - spriteP->curFrameP->collisionInset.bottom)
#define SW_COLLISION_RECT_RIGHT(spriteP) \
    (spriteP->destFrameRect.right - spriteP->curFrameP->collisionInset.right)

extern RgnHandle gSWCollisionSpareRgn;


/* Scrolling.c (mac only) */
typedef void (*WorldMoveProcPtr)(SpriteWorldPtr spriteWorldP, SpritePtr followSpriteP);
typedef void (*DoubleDrawProcPtr)(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRectA,
                                  Rect *dstRectA, Rect *srcRectB, Rect *dstRectB);

/* SpriteWorld.c's functions the win port never had */
typedef void (*CleanUpCallBackPtr)(void);
extern CleanUpCallBackPtr gSWCleanUpCallBackP;
extern Rect mainDeviceInitBounds;

/* Tiling.c (mac only) */

typedef short **TileMapPtr;

typedef struct TileMapStruct {
    short numRows;
    short numCols;
    TileMapPtr tileMap;
    Handle tileMapDataH;
    Handle arrayOfPointersH;
    Boolean isLocked;
} TileMapStruct, *TileMapStructPtr;

typedef void (*TileChangeProcPtr)(SpriteWorldPtr spriteWorldP);
typedef void (*TileRectDrawProcPtr)(SpriteWorldPtr spriteWorldP, Rect *updateRectP,
                                    Boolean optimizingOn);

#define botRight(r) (((Point *)&(r))[1])
#endif

#ifdef __cplusplus
}
#endif

#endif /* SPRITEWORLD_H */
