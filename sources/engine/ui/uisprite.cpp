/* uisprite.cpp -- sprites drawn into the interface, on scaled frames stretched smoothly. */
#include <math.h>
#include <vector>
#include "spriteworld.h"
#include "uiscale.h"

namespace UI {

namespace {

constexpr unsigned short kBlack = 0x0000;
constexpr unsigned short kWhite = 0x7fff;

/* A scratch frame at least of the sprite's size, its top-left part filled with one colour. The frame is kept
   for the next sprite and only made anew when a bigger one comes. */
SWFrameRec *Backdrop(SWFrameRec *&frame, int width, int height, unsigned short colour)
{
    Rect bounds;

    if (frame != NULL && (frame->frameRect.right < width || frame->frameRect.bottom < height))
        SWDisposeFrame(&frame);
    if (frame == NULL) {
        SetRect(&bounds, 0, 0, width, height);
        if (SWCreateFrame(GetGDevice(), &frame, &bounds, 16, 1) != 0 || frame == NULL) {
            frame = NULL;
            return NULL;
        }
        SWLockFrame(frame);
    }
    for (int y = 0; y < height; y++) {
        unsigned short *row = (unsigned short *)(frame->frameBaseAddr + (size_t)y * frame->frameRowBytes);
        for (int x = 0; x < width; x++)
            row[x] = colour;
    }
    return frame;
}

unsigned short PixelAt(const SWFrameRec *frame, int x, int y)
{
    return ((const unsigned short *)(frame->frameBaseAddr + (size_t)y * frame->frameRowBytes))[x];
}

/* Where a device row or column samples the sprite: the first of the two pixels blended and the weight of the
   second, in 1/256. */
struct Tap {
    int first;
    int last;
    int weight;
};

Tap TapAt(int device, int deviceSize, int spriteSize)
{
    double u = (device + 0.5) * spriteSize / deviceSize - 0.5;
    int first = (int)floor(u);
    Tap tap;

    tap.weight = (int)((u - first) * 256 + 0.5);
    tap.first = first < 0 ? 0 : first;
    tap.last = first + 1 >= spriteSize ? spriteSize - 1 : first + 1;
    if (first < 0)
        tap.last = 0;
    return tap;
}

} // namespace

void DrawSprite(DrawProcPtr draw, SWFrameRec *sprite, SWFrameRec *dst, Rect *srcRect, Rect *dstRect)
{
    if (dst == NULL || dst->framePort == NULL || !Mapping(dst->framePort->pix).Scaled()) {
        draw(sprite, dst, srcRect, dstRect);
        return;
    }
    int width = dstRect->right - dstRect->left;
    int height = dstRect->bottom - dstRect->top;
    if (width <= 0 || height <= 0)
        return;

    static SWFrameRec *blackFrame;
    static SWFrameRec *whiteFrame;
    SWFrameRec *overBlack = Backdrop(blackFrame, width, height, kBlack);
    SWFrameRec *overWhite = Backdrop(whiteFrame, width, height, kWhite);
    if (overBlack == NULL || overWhite == NULL)
        return;
    Rect local;
    SetRect(&local, 0, 0, width, height);
    draw(sprite, overBlack, srcRect, &local);
    draw(sprite, overWhite, srcRect, &local);

    const PixMap &pix = dst->framePort->pix;
    Rect area = Mapping(pix).DeviceRect(*dstRect);
    int areaWidth = area.right - area.left;
    int areaHeight = area.bottom - area.top;
    int left = area.left < 0 ? 0 : area.left;
    int right = area.right > pix.width ? pix.width : area.right;
    if (areaWidth <= 0 || areaHeight <= 0 || left >= right)
        return;
    std::vector<Tap> columns(right - left);
    for (int x = left; x < right; x++)
        columns[x - left] = TapAt(x - area.left, areaWidth, width);

    /* Over black a sprite pixel shows its colour, over white its colour plus 31 times how much of the frame
       under it shows through; both blended from the four pixels around each device pixel, in 1/65536. */
    for (int y = area.top < 0 ? 0 : area.top; y < area.bottom && y < pix.height; y++) {
        unsigned short *row = (unsigned short *)((char *)pix.baseAddr + (size_t)y * pix.rowBytes);
        Tap tapY = TapAt(y - area.top, areaHeight, height);
        for (int x = left; x < right; x++) {
            const Tap &tapX = columns[x - left];
            unsigned short black[4] = {
                PixelAt(overBlack, tapX.first, tapY.first), PixelAt(overBlack, tapX.last, tapY.first),
                PixelAt(overBlack, tapX.first, tapY.last), PixelAt(overBlack, tapX.last, tapY.last) };
            unsigned short white[4] = {
                PixelAt(overWhite, tapX.first, tapY.first), PixelAt(overWhite, tapX.last, tapY.first),
                PixelAt(overWhite, tapX.first, tapY.last), PixelAt(overWhite, tapX.last, tapY.last) };
            if ((black[0] | black[1] | black[2] | black[3]) == kBlack
                    && (white[0] & white[1] & white[2] & white[3]) == kWhite)
                continue;                       /* not drawn on */
            int weight[4] = {
                (256 - tapX.weight) * (256 - tapY.weight), tapX.weight * (256 - tapY.weight),
                (256 - tapX.weight) * tapY.weight, tapX.weight * tapY.weight };
            unsigned old = row[x];
            unsigned result = 0;
            for (int shift = 10; shift >= 0; shift -= 5) {
                int colour = 0;
                int through = 0;
                for (int k = 0; k < 4; k++) {
                    int b = (black[k] >> shift) & 31;
                    int w = (white[k] >> shift) & 31;
                    colour += weight[k] * b;
                    through += weight[k] * (w - b);
                }
                int level = (colour * 31 + (int)((old >> shift) & 31) * through + 31 * 32768) / (31 * 65536);
                result |= (unsigned)(level < 0 ? 0 : level > 31 ? 31 : level) << shift;
            }
            row[x] = (unsigned short)result;
        }
    }
}

} // namespace UI
