/* swcreatespriteworld.cpp -- SWCreateSpriteWorld (win 0x00477640, donor SpriteWorld.c line 117), its resize and dispose. Notes: Research/notes/swcreatespriteworld.md */
#include "burger.h"
#include "spriteworld.h"


// FUNCTION: EVNOVA 0x00477640
// MAC: _SWCreateSpriteWorld i386 0x000b6b92 ppc 0x000c26d8
short SWCreateSpriteWorld(SpriteWorldRec **spriteWorldP, CGrafPtr windowPort, const Rect &viewRect)
{
    short err;
    SpriteWorldRec *tempSpriteWorldP;

    err = noErr;
    *spriteWorldP = 0;

    tempSpriteWorldP = (SpriteWorldRec *)AllocAPointerClear(sizeof(SpriteWorldRec));
    if (tempSpriteWorldP == 0)
        err = memFullErr;

    if ((int)err == noErr) {
        err = SWCreateSpriteLayer(&tempSpriteWorldP->deadSpriteLayerP);

        if ((int)err != noErr)
            DeallocAPointer(tempSpriteWorldP);
    }

    if ((int)err == noErr) {
        tempSpriteWorldP->pixelDepth = 16;
        tempSpriteWorldP->windowPort = windowPort;
        tempSpriteWorldP->windRect = viewRect;
        tempSpriteWorldP->fastBlit = 1;
        SWClearStickyError();
        *spriteWorldP = tempSpriteWorldP;
    }

    SWSetStickyIfError(err);
    return err;
}

/* The world's space view has a new size. */
void SWResizeSpriteWorld(SpriteWorldPtr spriteWorldP, const Rect &viewRect)
{
    spriteWorldP->windRect = viewRect;
}

/* A world from SWCreateSpriteWorld freed, its layers' sprites already gone; *spriteWorldP becomes NULL. */
void SWDisposeSpriteWorld(SpriteWorldPtr *spriteWorldP)
{
    if (*spriteWorldP == NULL)
        return;
    if ((*spriteWorldP)->deadSpriteLayerP != NULL)
        SWDisposeSpriteLayer(&(*spriteWorldP)->deadSpriteLayerP);
    DeallocAPointer(*spriteWorldP);
    *spriteWorldP = NULL;
}
