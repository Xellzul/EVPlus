/* uiart.cpp -- the interface's art as GPU images (uiart.h). */
#include <unordered_map>
#include "burger.h"
#include "canvas.h"
#include "pixel.h"
#include "scratch.h"
#include "uiart.h"

namespace UI {

namespace Art {

namespace {

enum class Kind : unsigned char { Picture, Icon, Pattern, Sprite };

/* one image made of a piece of art: its kind and device size */
struct Made {
    Kind kind;
    int width;
    int height;
    PlatImage *image;
};

std::unordered_map<const void *, std::vector<Made>> kept;
std::vector<uint32_t> own;              /* art at its own size */
std::vector<uint32_t> sized;            /* and at the size asked for */
std::vector<uint16_t> framePixels;      /* a sprite frame's pixels and where it is drawn */
std::vector<uint8_t> frameAlpha;

constexpr uint32_t kOpaque = 0xff000000u;

int Depth(const Image_t *art)
{
    return (art->DataType + 7) & ~7;
}

PlatImage *Find(const void *art, Kind kind, int width, int height)
{
    auto found = kept.find(art);

    if (found == kept.end())
        return NULL;
    for (const Made &made : found->second)
        if (made.kind == kind && made.width == width && made.height == height)
            return made.image;
    return NULL;
}

PlatImage *Keep(const void *art, Kind kind, int width, int height, PlatImage *image)
{
    gArtGone = Gone;
    if (image != NULL)
        kept[art].push_back({ kind, width, height, image });
    SpareScratch(own);
    SpareScratch(sized);
    SpareScratch(framePixels);
    SpareScratch(frameAlpha);
    return image;
}

PlatImage *Keep(const void *art, Kind kind, int width, int height, const uint32_t *pixels)
{
    return Keep(art, kind, width, height, PlatImageCreate32(width, height, pixels, width * (int)sizeof(uint32_t)));
}

void Drop(const void *art)
{
    auto found = kept.find(art);

    if (found == kept.end())
        return;
    Canvas::FlushAll();                 /* draws waiting may show the images */
    for (const Made &made : found->second)
        PlatImageDestroy(made.image);
    kept.erase(found);
}

/* to the nearest pixel as CopyBitsScale8 steps: 16.16 steps from half a step in */
void StretchNearest(const uint32_t *pixels, int width, int height, uint32_t *out, int toWidth, int toHeight)
{
    int32_t xStep = (width << 16) / toWidth;
    int32_t yStep = (height << 16) / toHeight;
    int32_t y = yStep >> 1;

    for (int row = 0; row < toHeight; row++, y += yStep) {
        const uint32_t *from = pixels + (size_t)(y >> 16) * width;
        int32_t x = xStep >> 1;
        for (int column = 0; column < toWidth; column++, x += xStep)
            *out++ = from[x >> 16];
    }
}

/* to the nearest pixel as an icon is stretched: each device pixel takes the pixel its corner lies in */
void StretchIcon(const uint32_t *pixels, int width, int height, uint32_t *out, int toWidth, int toHeight)
{
    for (int row = 0; row < toHeight; row++) {
        const uint32_t *from = pixels + (size_t)(row * height / toHeight) * width;
        for (int column = 0; column < toWidth; column++)
            *out++ = from[column * width / toWidth];
    }
}

/* a sprite frame's RLE tokens as pixels and where they are drawn; a run's byte count is padded to 4 */
bool DecodeTokens(const SWFrameRec *frame, int width, int height)
{
    const unsigned char *tokens = (const unsigned char *)frame->rleTokenStart;
    int row = -1;
    int x = 0;

    for (;;) {
        uint32_t token = *(const uint32_t *)tokens;
        uint32_t opcode = token >> kTokenShift;
        int32_t bytes = (int32_t)(token & kCountMask);
        tokens += 4;
        if (opcode == kEndShapeToken)
            break;
        if (opcode == kLineStartToken) {
            row++;
            x = 0;
            if (row >= height)
                break;
            continue;
        }
        int count = bytes / Pixel::kBytes;
        if (opcode == kDrawPixelsToken || opcode == kSingleColorToken) {
            const uint16_t *run = (const uint16_t *)tokens;
            for (int i = 0; i < count; i++) {
                if (row < 0 || x + i < 0 || x + i >= width)
                    continue;
                size_t at = (size_t)row * width + x + i;
                framePixels[at] = opcode == kDrawPixelsToken ? run[i] : run[i & 1];
                frameAlpha[at] = 1;
            }
            tokens += opcode == kDrawPixelsToken ? (bytes + 3) & ~3 : 4;
        } else if (opcode != kSkipPixelsToken) {
            break;
        }
        x += count;
    }
    return true;
}

/* a frame's 16-bit pixels, drawn where its mask (when it has one) is black */
bool DecodePixels(const SWFrameRec *frame, int width, int height)
{
    const PixMap *pix = frame->framePort != NULL ? &frame->framePort->pix : NULL;
    const PixMap *mask = frame->maskPort != NULL ? &frame->maskPort->pix : NULL;

    if (pix == NULL || pix->pixelSize != 16 || pix->baseAddr == NULL)
        return false;
    for (int y = 0; y < height && frame->frameRect.top + y < pix->height; y++) {
        int row = frame->frameRect.top + y;
        const uint16_t *from = Pixel::Row(*pix, row);
        const uint16_t *through = mask != NULL && mask->pixelSize == 16 && row < mask->height
                                  ? Pixel::Row(*mask, row) : NULL;
        for (int x = 0; x < width && frame->frameRect.left + x < pix->width; x++) {
            int column = frame->frameRect.left + x;
            if (through == NULL || column >= mask->width || through[column] == 0) {
                framePixels[(size_t)y * width + x] = from[column];
                frameAlpha[(size_t)y * width + x] = 1;
            }
        }
    }
    return true;
}

} // namespace

bool Pixels(const Image_t *art, bool keyZero, std::vector<uint32_t> &out)
{
    int width = art != NULL ? (int)art->Width : 0;
    int height = art != NULL ? (int)art->Height : 0;
    int depth = art != NULL ? Depth(art) : 0;

    if (width <= 0 || height <= 0 || art->ImagePtr == NULL || (depth == 8 && art->PalettePtr == NULL))
        return false;
    if (depth != 8 && depth != 16 && depth != 24 && depth != 32)
        return false;
    out.resize((size_t)width * height);
    uint32_t *to = out.data();
    for (int y = 0; y < height; y++) {
        const unsigned char *p = art->ImagePtr + (size_t)y * art->RowBytes;
        for (int x = 0; x < width; x++, p += depth / 8) {
            if (depth == 8) {
                const unsigned char *rgb = art->PalettePtr + *p * 3;
                *to++ = keyZero && *p == 0 ? 0 : kOpaque | (uint32_t)rgb[0] << 16 | (uint32_t)rgb[1] << 8 | rgb[2];
            } else if (depth == 16) {
                *to++ = Pixel::ToArgb(*(const uint16_t *)p);
            } else if (depth == 24) {
                *to++ = kOpaque | (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2];
            } else {
                *to++ = kOpaque | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
            }
        }
    }
    return true;
}

void Stretch(const uint32_t *pixels, int width, int height, const Rect &part, uint32_t *out, int toWidth,
             int toHeight)
{
    PixMap from = {};
    PixMap to = {};

    SetRect(&from.bounds, 0, 0, width, height);
    from.pixelSize = 32;
    from.baseAddr = (void *)pixels;
    from.width = width;
    from.height = height;
    from.rowBytes = width * 4;
    SetRect(&to.bounds, 0, 0, toWidth, toHeight);
    to.pixelSize = 32;
    to.baseAddr = out;
    to.width = toWidth;
    to.height = toHeight;
    to.rowBytes = toWidth * 4;
    if (part.right - part.left != toWidth || part.bottom - part.top != toHeight) {
        BlitScaleArgb(&from, &to, &part, &to.bounds);
        return;
    }
    for (int y = 0; y < toHeight; y++)
        for (int x = 0; x < toWidth; x++) {
            int column = part.left + x;
            int row = part.top + y;
            bool inside = column >= 0 && row >= 0 && column < width && row < height;
            out[(size_t)y * toWidth + x] = inside ? pixels[(size_t)row * width + column] : 0;
        }
}

bool PicturePixels(const Image_t *pic, int width, int height, std::vector<uint32_t> &out)
{
    Rect all;

    if (width <= 0 || height <= 0)
        return false;
    if (width == (int)pic->Width && height == (int)pic->Height)
        return Pixels(pic, false, out);
    if (!Pixels(pic, false, own))
        return false;
    out.resize((size_t)width * height);
    SetRect(&all, 0, 0, pic->Width, pic->Height);
    if (Depth(pic) == 8)
        StretchNearest(own.data(), pic->Width, pic->Height, out.data(), width, height);
    else
        Stretch(own.data(), pic->Width, pic->Height, all, out.data(), width, height);
    SpareScratch(own);
    return true;
}

PlatImage *Picture(const Image_t *pic, int width, int height)
{
    PlatImage *image = Find(pic, Kind::Picture, width, height);

    if (image != NULL || pic == NULL || !PicturePixels(pic, width, height, sized))
        return image;
    return Keep(pic, Kind::Picture, width, height, sized.data());
}

PlatImage *Icon(const Image_t *icon, int width, int height)
{
    PlatImage *image = Find(icon, Kind::Icon, width, height);

    if (image != NULL || width <= 0 || height <= 0 || !Pixels(icon, true, own))
        return image;
    if (width == (int)icon->Width && height == (int)icon->Height)
        return Keep(icon, Kind::Icon, width, height, own.data());
    sized.resize((size_t)width * height);
    StretchIcon(own.data(), icon->Width, icon->Height, sized.data(), width, height);
    return Keep(icon, Kind::Icon, width, height, sized.data());
}

PlatImage *Pattern(const Image_t *pattern)
{
    PlatImage *image = pattern != NULL ? Find(pattern, Kind::Pattern, pattern->Width, pattern->Height) : NULL;

    if (image != NULL || !Pixels(pattern, false, own))
        return image;
    return Keep(pattern, Kind::Pattern, pattern->Width, pattern->Height, own.data());
}

PlatImage *Sprite(const SWFrameRec *frame)
{
    int width = frame != NULL ? frame->frameRect.right - frame->frameRect.left : 0;
    int height = frame != NULL ? frame->frameRect.bottom - frame->frameRect.top : 0;
    PlatImage *image = Find(frame, Kind::Sprite, width, height);

    if (image != NULL || width <= 0 || height <= 0)
        return image;
    framePixels.assign((size_t)width * height, 0);
    frameAlpha.assign((size_t)width * height, 0);
    if (!(frame->rleTokenStart != NULL ? DecodeTokens(frame, width, height) : DecodePixels(frame, width, height)))
        return NULL;
    return Keep(frame, Kind::Sprite, width, height,
                PlatImageCreate(width, height, framePixels.data(), frameAlpha.data()));
}

PlatImage *SpriteMade(const SWFrameRec *frame)
{
    return frame != NULL ? Find(frame, Kind::Sprite, frame->frameRect.right - frame->frameRect.left,
                                frame->frameRect.bottom - frame->frameRect.top)
                         : NULL;
}

void Gone(const Image_t *art)
{
    Drop(art);
}

void FrameGone(const SWFrameRec *frame)
{
    Drop(frame);
}

/* the sprite frames' stay: the space view's draws show them */
void Clear()
{
    Canvas::FlushAll();
    for (auto entry = kept.begin(); entry != kept.end();) {
        std::vector<Made> &made = entry->second;
        for (size_t i = made.size(); i-- > 0;) {
            if (made[i].kind == Kind::Sprite)
                continue;
            PlatImageDestroy(made[i].image);
            made.erase(made.begin() + i);
        }
        entry = made.empty() ? kept.erase(entry) : std::next(entry);
    }
}

} // namespace Art

} // namespace UI
