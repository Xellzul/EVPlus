// COMPILE: -lang c++ -bool off
/* swfiximagegworld.c -- SWFixImageGWorld (win 0x00479260, SpriteWorldUtils.c line 1212, donor SpriteWorldUtils.c line 1099). Notes: Research/notes/swfiximagegworld.md */
/* kClear is all-ones (-1) and kSolid zero, one constant each: the exe stores 0xffff and
   0xffffffff and the truncation does the rest. The exe also returned early at 8 bits and
   shallower, and had an unreachable `case 8:'; the game draws only at 16 bits. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define paramErr    (-50)
#undef kClear
#undef kSolid
#define kClear     (-1)
#define kSolid      0

// FUNCTION: EVNOVA 0x00479260
// MAC: _SWFixImageGWorld i386 0x000a820c ppc 0x000b3028
short SWFixImageGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP)
{
    CGrafPtr saveGWorld;
    void *saveDevice;
    char *baseAddr;
    char *maskAddr;
    int32_t rowBytes;
    int32_t width, height;
    int32_t depth;
    int32_t x, y;
    unsigned short transparentValue;
    PixMapPtr pictPix;
    PixMapPtr maskPix;
    short err = noErr;

    pictPix = GetCWindowPixMap(pictureGWorld);
    maskPix = GetCWindowPixMap(maskGWorld);

    if (GetPixDepth(pictPix) != GetPixDepth(maskPix))
        SWAssertFail("SpriteWorldUtils.c", 1212);

    if (rectP == 0)
        rectP = GetWindowPortRect(pictureGWorld);

    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    depth = GetPixDepth(pictPix);
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    GetPort(&saveGWorld);
    saveDevice = GetGWorldDevice(saveGWorld);

    baseAddr = GetPixBaseAddr(pictPix);
    maskAddr = GetPixBaseAddr(maskPix);
    rowBytes = GetPixRowBytes(pictPix);

    baseAddr += rectP->top * rowBytes;
    baseAddr += (rectP->left * depth) >> 3;
    maskAddr += rectP->top * rowBytes;
    maskAddr += (rectP->left * depth) >> 3;
    rowBytes -= (width * depth) >> 3;

    switch (depth) {
        case 16:
            {
                unsigned short *dst = (unsigned short *)baseAddr;
                unsigned short *mask = (unsigned short *)maskAddr;

                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        if (*mask) {
                            *dst++ = 0;
                            *mask++ = kClear;
                        } else {
                            dst++;
                            *mask++ = kSolid;
                        }
                    }
                    dst = (unsigned short *)((char *)dst + rowBytes);
                    mask = (unsigned short *)((char *)mask + rowBytes);
                }
            }
            break;

        case 32:
            {
                uint32_t *dst = (uint32_t *)baseAddr;
                uint32_t *mask = (uint32_t *)maskAddr;

                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        if (*mask) {
                            *dst++ = 0;
                            *mask++ = kClear;
                        } else {
                            dst++;
                            *mask++ = kSolid;
                        }
                    }
                    dst = (uint32_t *)((char *)dst + rowBytes);
                    mask = (uint32_t *)((char *)mask + rowBytes);
                }
            }
            break;

        default:
            err = paramErr;
    }

    SetPort(saveGWorld);

    return err;
}

