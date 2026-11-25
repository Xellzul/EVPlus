/* swcreatespritelayer.cpp -- SWCreateSpriteLayer (win 0x004770d0, donor SpriteLayer.c line 39) and SWDisposeSpriteLayer (win 0x00477120, donor line 73). Notes: Research/notes/swcreatespritelayer.md */
#include "burger.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x004770d0
// MAC: _SWCreateSpriteLayer i386 0x000ac560 ppc 0x000b7640
short SWCreateSpriteLayer(SpriteLayerRec **spriteLayerP)
{
    short err;
    SpriteLayerRec *tempSpriteLayerP;

    err = 0;
    *spriteLayerP = 0;

    tempSpriteLayerP = (SpriteLayerRec *)AllocAPointerClear(sizeof(SpriteLayerRec));

    if (tempSpriteLayerP != 0) {
        *spriteLayerP = tempSpriteLayerP;
    } else {
        err = memFullErr;
    }

    return err;
}

// FUNCTION: EVNOVA 0x00477120
// MAC: _SWDisposeSpriteLayer i386 0x000ac5ba ppc 0x000b76c0
void SWDisposeSpriteLayer(SpriteLayerRec **spriteLayerPP)
{
    SpriteLayerRec *spriteLayerP = *spriteLayerPP;

    if (spriteLayerP != 0) {
        DeallocAPointer(spriteLayerP);
        *spriteLayerPP = 0;
    }
}
