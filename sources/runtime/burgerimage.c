// COMPILE: -lang c++ -bool off
/* burgerimage.c -- BurgerLib ImImageManager: freeing an image and remapping its indexed pixels. Notes: Research/notes/burgerimage.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ImageDelete (BurgerLib ImImageManager). STATE: MATCH 100.0%, exe 13 / obj 13,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fdb60
void ImageDelete(struct Image_t *ImagePtr)
{
    if (ImagePtr != 0) {
        ImageDestroy(ImagePtr);
        DeallocAPointer(ImagePtr);
    }
}

/* ImageDestroy (BurgerLib ImImageManager). STATE: MATCH 100.0%, exe 23 / obj 23,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fdb90
void ImageDestroy(struct Image_t *ImagePtr)
{
    if (ImagePtr != 0) {
        DeallocAPointer(ImagePtr->ImagePtr);
        DeallocAPointer(ImagePtr->PalettePtr);
        DeallocAPointer(ImagePtr->AlphaPtr);
        FastMemSet(ImagePtr, 0, sizeof(Image_t));
    }
}

/* ImageRemapIndexed (BurgerLib ImImageManager). STATE: MATCH 100.0%, exe 41 / obj 41,
   byte-identical yes (the jump table is data, not read by the gate). OPEN: none */
// FUNCTION: EVNOVA 0x004fdbf0
void ImageRemapIndexed(struct Image_t *ImagePtr, const Word8 *RemapPtr)
{
    Word8 *DestPtr;
    Word8 *WorkPtr;
    Word Width;
    Word Height;

    DestPtr = ImagePtr->ImagePtr;
    Width = ImagePtr->Width;
    Height = ImagePtr->Height;
    if (DestPtr != 0 && Width != 0 && Height != 0) {
        switch (ImagePtr->DataType) {
        case IMAGE8_PAL_ALPHA_PAL:
        case IMAGE8_PAL_ALPHA:
        case IMAGE8_PAL:
            do {
                WorkPtr = DestPtr;
                do {
                    WorkPtr[0] = RemapPtr[WorkPtr[0]];
                    ++WorkPtr;
                } while (--Width);
                Width = ImagePtr->Width;
                DestPtr += ImagePtr->RowBytes;
            } while (--Height);
        }
    }
}

#ifdef __cplusplus
}
#endif
