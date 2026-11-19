// COMPILE: -lang c++ -bool off
/* blitpixierleparsepixels.c -- BlitPixieRLEParsePixels (win 0x00472510). Notes: Research/notes/blitpixierleparsepixels.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"

EXTERN_C_BEGIN

#define kParamErr       -50
#define kMemFullErr     -108

#define RLE_ROW         0x1000000
#define RLE_LITERAL     0x2000000
#define RLE_TRANSPARENT 0x3000000
#define RLE_FILL        0x4000000


// FUNCTION: EVNOVA 0x00472510
// MAC: _BlitPixieRLEParsePixels i386 0x000c2875 ppc 0x000cec48
short BlitPixieRLEParsePixels(unsigned char *srcPixels, unsigned char *maskPixels,
        uint32_t rowBytes, unsigned short byteWidth, unsigned short numRows,
        unsigned short depth, void ***frameData)
{
    void **frameHandle;
    int32_t *rowHeader;
    unsigned char *srcRow;
    unsigned char *maskRow;
    unsigned char *srcPixel;
    unsigned char *maskPixel;
    uint32_t row;
    int32_t *runToken;
    int32_t runCount;
    int32_t bytesPerPixel;
    int32_t sameCount;
    int32_t depthL;
    uint32_t lastPixel;
    uint32_t maskValue;
    uint32_t pixelValue;
    uint32_t width;
    uint32_t rows;
    unsigned char *tokens;
    uint32_t x;
    unsigned char state;
    int32_t size;
    short err;

    *frameData = 0;

    if (depth != 16 && depth != 32)
        return kParamErr;

    if (byteWidth == 0 || numRows == 0)
        return kParamErr;

    rows = numRows;
    depthL = depth;
    bytesPerPixel = depthL >> 3;

    frameHandle = (void **)AllocAHandle2(
            numRows * byteWidth * 8 + rows * 4 + 4, 0);
    if (frameHandle != 0) {
        LockAHandle((Handle)frameHandle);
        tokens = (unsigned char *)*frameHandle;

        srcRow = srcPixels;
        maskRow = maskPixels;

        row = 0;
        if (row < rows) {
            width = byteWidth;
            do {
                srcPixel = srcRow;

            rowHeader = (int32_t *)tokens;
            tokens += 4;

            runCount = 0;
            state = 0;
            sameCount = 0;
            maskPixel = maskRow;

            for (x = 0; x < width; x += bytesPerPixel) {
                switch (depthL) {
                case 16:
                    maskValue = *(unsigned short *)maskPixel;
                    break;
                case 32:
                    maskValue = *(uint32_t *)maskPixel;
                    break;
                }

                if (maskValue != 0) {
                    sameCount = 0;

                    switch (state) {
                    case 1:
                        runCount += bytesPerPixel;
                        break;

                    case 2:
                        *runToken = runCount + RLE_LITERAL;
                        tokens += (-(intptr_t)tokens) & 3;
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;

                    case 3:
                        *runToken = runCount + RLE_FILL;
                        if (depth < 32)
                            lastPixel |= lastPixel << 16;
                        runToken[1] = lastPixel;
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;

                    case 0:
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 1;
                        runCount = bytesPerPixel;
                        break;
                    }
                } else {
                    switch (depthL) {
                    case 16:
                        pixelValue = *(unsigned short *)srcPixel;
                        break;
                    case 32:
                        pixelValue = *(uint32_t *)srcPixel;
                        break;
                    }

                    if (sameCount != 0 && pixelValue == lastPixel)
                        sameCount += bytesPerPixel;
                    else
                        sameCount = bytesPerPixel;

                    switch (state) {
                    case 2:
                        runCount += bytesPerPixel;
                        break;

                    case 3:
                        *runToken = runCount + RLE_FILL;
                        if (depth < 32)
                            lastPixel |= lastPixel << 16;
                        runToken[1] = lastPixel;
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;

                    case 1:
                        *runToken = runCount + RLE_TRANSPARENT;
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;

                    case 0:
                        runToken = (int32_t *)tokens;
                        tokens += 4;
                        state = 2;
                        runCount = bytesPerPixel;
                        break;
                    }

                    switch (depthL) {
                    case 16:
                        *(unsigned short *)tokens = pixelValue;
                        tokens += 2;
                        break;
                    case 32:
                        *(uint32_t *)tokens = pixelValue;
                        tokens += 4;
                        break;
                    }

                    lastPixel = pixelValue;
                }

                srcPixel += bytesPerPixel;
                maskPixel += bytesPerPixel;
            }

            switch (state) {
            case 1:
                tokens = (unsigned char *)runToken;
                break;

            case 2:
                *runToken = runCount + RLE_LITERAL;
                tokens += (-(intptr_t)tokens) & 3;
                break;

            case 3:
                *runToken = runCount + RLE_FILL;
                if (depth < 32)
                    lastPixel |= lastPixel << 16;
                runToken[1] = lastPixel;
                runToken += 2;
                break;
            }

            *rowHeader = (int32_t)((tokens - 4) - (unsigned char *)rowHeader) + RLE_ROW;

            srcRow += rowBytes;
            maskRow += rowBytes;
            row++;
        } while (row < rows);
        }

        *(int32_t *)tokens = 0;

        size = (tokens + 4) - (unsigned char *)*frameHandle;

        ResizeAHandle((Handle)frameHandle, size);

        err = MemError();
        if ((int)err == noErr)
            *frameData = frameHandle;
    } else {
        err = kMemFullErr;
    }

    return err;
}

EXTERN_C_END
