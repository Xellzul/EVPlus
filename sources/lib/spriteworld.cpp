/* spriteworld.cpp -- SpriteWorld's SpriteWorld.c, tu-map object O17: the sprite world, its update and animation. Notes: Research/notes/spriteworld.md */

#include "game.h"

// FUNCTION: EVNOVA 0x00477b00
// MAC: _SWAddSpriteLayer i386 0x000b71b0 ppc 0x000c2cf4
void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *newSpriteLayerP)
{
    SpriteLayerRec *tailSpriteLayerP = spriteWorldP->tailSpriteLayerP;

    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 459, 444);
    if (tailSpriteLayerP != NULL)
        tailSpriteLayerP->nextSpriteLayerP = newSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = newSpriteLayerP;
    newSpriteLayerP->nextSpriteLayerP = NULL;
    spriteWorldP->tailSpriteLayerP = newSpriteLayerP;
}

#if EVPLUS_TEST_PAGES
// FUNCTION: EVNOVA 0x00477c70
// MAC: _SWSetPortToWindow i386 0x000b74e4 ppc 0x000c3098
void SWSetPortToWindow(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld != NULL && spriteWorld->windowPort != NULL)
        SetPort(spriteWorld->windowPort);
}
#endif


/* The game's drawing of the world, called by Animate and Update. */
void (*gSWDraw)(SpriteWorldRec *spriteWorldP);

// FUNCTION: EVNOVA 0x00477fa0
// MAC: _SWUpdateSpriteWorld i386 0x000b9062 ppc 0x000c4ff0
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1440);

    SWIsWindowInForeground(spriteWorldP);
    if (gSWDraw != 0)
        (*gSWDraw)(spriteWorldP);
}


// FUNCTION: EVNOVA 0x004781f0
// MAC: _SWProcessSpriteWorld i386 0x000b8957 ppc 0x000c485c
void SWProcessSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1618);

    spriteWorldP->frameHasOccurred = 1;

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0) {
                if (!curSpriteP->curFrameP->isFrameLocked)
                    SWLockFrame(curSpriteP->curFrameP);
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    SWIsWindowInForeground(spriteWorldP);
}


// FUNCTION: EVNOVA 0x00478480
// MAC: _SWAnimateSpriteWorld i386 0x000b95e6 ppc 0x000c5550
void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1984);

    if (!spriteWorldP->frameHasOccurred)
        return;

    if (gSWDraw != 0)
        (*gSWDraw)(spriteWorldP);
}

// FUNCTION: EVNOVA 0x00478e20
// MAC: _SWIsWindowInForeground i386 0x000b88cd ppc 0x000c47c8
Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP)
{
    spriteWorldP->fastBlit = 1;

    return spriteWorldP->fastBlit;
}

// FUNCTION: EVNOVA 0x00478e50
// MAC: _SWHideAllSprites i386 0x000b8c09 ppc 0x000c4ba4
void SWHideAllSprites(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SpriteWorld.c", 2847, 2823);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0 && curSpriteP->isVisible)
                SWSetSpriteVisible(curSpriteP, 0);

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}
