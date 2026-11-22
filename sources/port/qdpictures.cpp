/* qdpictures.cpp -- PICT, cicn and ppat: loading (PICTs through the gamma ramp, the ppat decoder) and drawing */
#include "pixel.h"
#include "scaleddraw.h"
#include "burger.h"

// FUNCTION: EVNOVA 0x004b8ed0
PicHandle LoadPictResource(int picID, char gamma)
{
    char **h;
    PicHandle img;
    int32_t err;
    int32_t advance;
    unsigned char *p;
    unsigned short pix;
    int32_t bytes;
    int32_t pixels;
    int32_t rows;

    h = GetResource(kPictType, picID);
    if (h != NULL) {
        img = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = ImageParsePict(img, (unsigned char *)*h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(img);
            return NULL;
        }
        if (gamma != 0 && img->Width != 0 && img->Height != 0) {
            switch ((img->DataType + 7) & 0xfffffff8) {
            case 8:
                p = img->PalettePtr;
                if (p != NULL) {
                    bytes = 256 * 3;
                    do {
                        *p = (gGammaRamp[(*p >> 3) & 0x1f] << 3);
                        p++;
                    } while (--bytes != 0);
                }
                break;
            case 16:
                p = img->ImagePtr;
                advance = img->RowBytes - img->Width * 2;
                rows = img->Height;
                do {
                    pixels = img->Width;
                    do {
                        pix = *(unsigned short *)p;
                        *(unsigned short *)p = Pixel::Map(pix, gGammaRamp);
                        p += 2;
                    } while (--pixels != 0);
                    p += advance;
                } while (--rows != 0);
                break;
            case 24:
                p = img->ImagePtr;
                advance = img->RowBytes - img->Width * 3;
                rows = img->Height;
                do {
                    bytes = img->Width * 3;
                    do {
                        *p = (gGammaRamp[(*p >> 3) & 0x1f] << 3);
                        p++;
                    } while (--bytes != 0);
                    p += advance;
                } while (--rows != 0);
                break;
            }
        }
        return img;
    }
    return NULL;
}

// FUNCTION: EVNOVA 0x004b9050
PicHandle GetPicture(int picID)
{
    return LoadPictResource(picID, 0);
}

/*
 * The same one-line wrapper at the next address, passing 0 where GetPicture
 * passes 1. Ghidra's decompilation prints both immediates as 0; the exe bytes
 * say otherwise, which is why this pair is worth the note.
 */
// FUNCTION: EVNOVA 0x004b9060
PicHandle GetPictureNoGamma(int picID)
{
    return LoadPictResource(picID, 0);
}

/* A decoded picture as a CopyBits source. The exe passed a record of its own (bounds, then the bit depth
   where a PixMap has pixelSize, then the Image_t) that only lined up with a PixMap's fields because every
   pointer and integer there was 4 bytes; this fills a PixMap. (The exe's record also put AlphaPtr where
   originV is; nothing reads originV.) */
static void ImageAsPixMap(PixMap *pm, const Image_t *im)
{
    SetRect(&pm->bounds, 0, 0, im->Width, im->Height);
    pm->pixelSize = (im->DataType + 7) & ~7;
    pm->baseAddr = im->ImagePtr;
    pm->ctab = im->PalettePtr;
    pm->originV = 0;
    pm->width = im->Width;
    pm->height = im->Height;
    pm->rowBytes = im->RowBytes;
}

/*
 * Blits a decoded picture straight into the current port. Name invented - the
 * mac has no twin, since on that side the caller would have had a real PixMap
 * already.
 */
// FUNCTION: EVNOVA 0x004b9070
void DrawPicHandle(PicHandle pic, Rect *dstRect)
{
    PixMap src;

    if (pic == NULL || dstRect == NULL)
        return;
    if (CurrentGrafPort == NULL)
        return;

    ImageAsPixMap(&src, pic);
    CopyBits((BitMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
             &src.bounds, dstRect, 0, NULL);
}

/*
 * Named apart from the resource manager's ReleaseResource at 0x004ce370,
 * which is a different function; the other TUs still declare this one under
 * the shared name and want reconciling.
 */
// FUNCTION: EVNOVA 0x004b9130
void QDReleaseResource(PicHandle pic)
{
    ImageDelete(pic);
}

// FUNCTION: EVNOVA 0x004b9140
void QDGetPictureBounds(PicHandle pic, Rect *r)
{
    SetRect(r, 0, 0, pic->Width, pic->Height);
}

// FUNCTION: EVNOVA 0x004bbb80
Image_t *GetCIcon(short id)
{
    char **h;
    Image_t *icon;
    int err;

    h = GetResource('cicn', id);
    if (h != NULL) {
        icon = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = ImageParseCicn(icon, (Word8 *)*h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(icon);
            return NULL;
        }
        return icon;
    }
    return NULL;
}

// FUNCTION: EVNOVA 0x004bbbf0
void nv_BlitImageOr(Rect *dstRect, Image_t *icon)
{
    PixMap src;

    if (icon == NULL || dstRect == NULL)
        return;
    if (CurrentGrafPort == NULL)
        return;

    ImageAsPixMap(&src, icon);
    CopyBits((BitMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
             &src.bounds, dstRect, 1, NULL);
}

// FUNCTION: EVNOVA 0x004bbcb0
void DisposeCIcon(Image_t *icon)
{
    ImageDelete(icon);
}

// FUNCTION: EVNOVA 0x004bbcc0
void QDGetCIconBounds(Image_t *icon, Rect *r)
{
    SetRect(r, 0, 0, icon->Width, icon->Height);
}

#define kPixPatFullColor    1                   /* patType */

// BUG (CE F18): the swaps shift the signed value right, so a field whose last byte is >= 0x80
// reads with 0xFF above that byte; such a plug-in ppat fails validation, gets a black
// palette (ctSize >= 128) or writes before the palette (a colour index >= 128)
#define SWAP16(val) (val >> 8 | val << 8)
#define SWAP32(val) (val >> 24 | (val & 0xff0000) >> 8 | (val & 0xff00) << 8 | val << 24)

static short ReadShort(const void *data)
{
    return SWAP16(*(const short *)data);
}

static int32_t ReadLong(const void *data)
{
    return SWAP32(*(const int32_t *)data);
}

/* The PixPat header, its PixMap, the pixels expanded to one byte each and the colour
   table as RGB triplets: an IMAGE8_PAL image. Returns 0, or 1 with the image released.
   As CE's decoder has it: the offsets are taken as shorts, ctSize as the entry count, and
   each colour component's low byte. */
static int CE_ImageParsePPat(Image_t *image, const void *ppat)
{
    const unsigned char *src;
    const unsigned char *map;
    const unsigned char *data;
    const unsigned char *ctab;
    unsigned char *dst;
    Word8 *palette;
    unsigned short rowBytes;
    short mapOffset;
    short dataOffset;
    short depth;
    short count;
    short index;
    int32_t width;
    int32_t height;
    int32_t ctabOffset;
    int perByte;
    int mask;
    int x;
    int y;
    int i;

    memset(image, 0, sizeof(Image_t));
    src = (const unsigned char *)ppat;
    mapOffset = ReadLong(src + 2);
    dataOffset = ReadLong(src + 6);
    if (ReadShort(src) == kPixPatFullColor && mapOffset > 0 && dataOffset > 0) {
        map = src + mapOffset;
        rowBytes = ReadShort(map + 4);
        height = ReadShort(map + 10) - ReadShort(map + 6);
        width = ReadShort(map + 12) - ReadShort(map + 8);
        depth = ReadShort(map + 32);
        ctabOffset = ReadLong(map + 42);
        if ((rowBytes & kPixMapFlagMask) == kPixMapFlag && width > 0 && height > 0
                && ReadShort(map + 14) == 0 && ReadShort(map + 16) == 0
                && (depth == 1 || depth == 2 || depth == 4 || depth == 8)
                && ctabOffset > 0) {
            data = src + dataOffset;
            dst = (unsigned char *)AllocAPointer(width * height);
            image->ImagePtr = dst;
            image->Width = width;
            image->Height = height;
            image->RowBytes = width;
            image->DataType = IMAGE8_PAL;

            rowBytes &= kRowBytesMask;
            perByte = 8 / depth;
            mask = (1 << depth) - 1;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++)
                    *dst++ = data[y * rowBytes + x / perByte]
                             >> (8 - depth - x % perByte * depth) & mask;
            }

            ctab = src + ctabOffset;
            palette = (Word8 *)AllocAPointer(kPaletteBytes);
            memset(palette, 0, kPaletteBytes);
            image->PalettePtr = palette;
            count = ReadShort(ctab + 6);
            ctab += 8;
            for (i = 0; i < count; i++) {
                index = ReadShort(ctab);
                if (index < 256) {
                    palette[index * 3] = ctab[3];
                    palette[index * 3 + 1] = ctab[5];
                    palette[index * 3 + 2] = ctab[7];
                }
                ctab += 8;
            }
            return 0;
        }
    }
    ImageDestroy(image);
    return 1;
}

// FUNCTION: EVNOVA 0x004bbd40
void DisposePixPat(Image_t *pat)
{
    ImageDelete(pat);
}

// FUNCTION: EVNOVA 0x004bbd50
Image_t *GetPixPat(short id)
{
    char **h;
    Image_t *pat;
    int err;

    h = GetResource('ppat', id);
    if (h != NULL) {
        pat = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = CE_ImageParsePPat(pat, *h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(pat);
            return NULL;
        }
        return pat;
    }
    return NULL;
}

// FUNCTION: EVNOVA 0x004bbdc0
void FillCRect(Rect *dstRect, Image_t *pat)
{
    Rect srcR;
    Rect dstR;
    int x;
    PixMap src;
    unsigned int cols;
    unsigned int rows;
    unsigned int tileW;
    unsigned int tileH;
    int y;
    unsigned int col;

    if (CurrentGrafPort == NULL)
        return;
    tileW = pat->Width;
    cols = dstRect->right - dstRect->left;
    cols = cols + tileW;
    cols = cols - 1;
    cols = cols / tileW;
    tileH = pat->Height;
    rows = dstRect->bottom - dstRect->top;
    rows = rows + tileH;
    rows = rows - 1;
    rows = rows / tileH;
    if (cols == 0 || rows == 0)
        return;

    ImageAsPixMap(&src, pat);
    UI::ArtComposition tiles(CurrentGrafPort, *dstRect);    /* the tiles stretched as one */
    y = 0;
    do {
        col = cols;
        x = 0;
        do {
            dstR.left = dstRect->left + x;
            dstR.right = dstR.left + pat->Width;
            dstR.top = dstRect->top + y;
            dstR.bottom = dstR.top + pat->Height;
            if (dstR.right > dstRect->right)
                dstR.right = dstRect->right;
            if (dstR.bottom > dstRect->bottom)
                dstR.bottom = dstRect->bottom;
            srcR.left = srcR.top = 0;
            srcR.right = dstR.right - dstR.left;
            srcR.bottom = dstR.bottom - dstR.top;
            CopyBits((BitMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
                     &srcR, &dstR, 0, NULL);
            x = x + pat->Width;
            col -= 1;
        } while (col != 0);
        y += pat->Height;
        rows -= 1;
    } while (rows != 0);
}
