/* swparticles.cpp -- SpriteWorld's SWParticles.c, the whole object (win 0x0047b7d0-0x0047c800). Notes: Research/notes/swparticles.md */

#include "game.h"
#include "burger.h"

static ParticlePtr gParticleArray = NULL;
static int gLowestUnusedElement;
static int32_t gLastElementInUse;
static int gNumberOfParticlesActive;
static int gGravity;
static uint32_t gNumArrayElements;
static Boolean gParticlesInitialized = false;
static int32_t scrollOffsetHoriz;
static int32_t scrollOffsetVert;

// FUNCTION: EVNOVA 0x0047b7e0
// MAC: _InitParticles i386 0x000c4f83 ppc 0x000d11c0
short InitParticles(int numElements, int gravity)
{
    short err;

    err = 0;
    gNumArrayElements = numElements;
    gGravity = gravity;
    numElements = numElements * sizeof(Particle);
    gParticleArray = (ParticlePtr)AllocAPointer(numElements);
    if (gParticleArray == NULL)
        err = memFullErr;
    else
        ClearParticles(0);
    gLowestUnusedElement = 0;
    gLastElementInUse = 0;
    gNumberOfParticlesActive = 0;
    scrollOffsetHoriz = scrollOffsetVert = 0;
    if ((int)err == 0)
        gParticlesInitialized = 1;
    else
        gParticlesInitialized = 0;
    return err;
}

// FUNCTION: EVNOVA 0x0047b870
// MAC: _DisposeParticles i386 0x000c4e80 ppc 0x000d1030
void DisposeParticles(void)
{
    if (gParticleArray != NULL) {
        DeallocAPointer(gParticleArray);
        gParticleArray = NULL;
        gLowestUnusedElement = 0;
        gLastElementInUse = 0;
        gNumberOfParticlesActive = 0;
    }
    gParticlesInitialized = 0;
}

// FUNCTION: EVNOVA 0x0047b8c0
// MAC: _ClearParticles i386 0x000c4ec8 ppc 0x000d10a4
void ClearParticles(Boolean active)
{
    int32_t i;

    if (gParticleArray == NULL)
        SW_ASSERT_FAIL("SWParticles.c", 0x5d, 0x5d);
    for (i = 0; i < gNumArrayElements; i++) {
        if (active != 0 && gParticleArray[i].lifeRemaining > 1)
            gParticleArray[i].lifeRemaining = 1;
        else
            gParticleArray[i].lifeRemaining = 0;
    }
    gLowestUnusedElement = 0;
    gLastElementInUse = 0;
    gNumberOfParticlesActive = 0;
}

// FUNCTION: EVNOVA 0x0047b940
// MAC: _SetParticleScrollOffset i386 0x000c5008 ppc 0x000d129c
void SetParticleScrollOffset(int h, int v)
{
    scrollOffsetHoriz = h;
    scrollOffsetVert = v;
}

/* Moves every live particle and the scroll offset by (h, v) in 24.8 fixed point (the system wrap). */
void ShiftParticles(int32_t h, int32_t v)
{
    int32_t i;

    if (!gParticlesInitialized)
        return;
    scrollOffsetHoriz += h / 256;
    scrollOffsetVert += v / 256;
    for (i = 0; i <= gLastElementInUse; i++) {
        if (gParticleArray[i].lifeRemaining > 0) {
            gParticleArray[i].horizLoc += h;
            gParticleArray[i].vertLoc += v;
            gParticleArray[i].oldHorizLoc += h;
            gParticleArray[i].oldVertLoc += v;
        }
    }
}

// FUNCTION: EVNOVA 0x0047b960
// MAC: _NewParticle i386 0x000c501d ppc 0x000d12b0
void NewParticle(int32_t color, int32_t horizLoc, int32_t vertLoc, int32_t horizSpeed,
                 int32_t vertSpeed, short shade, short lifeCount)
{
    if (!gParticlesInitialized)
        return;

    if (gNumberOfParticlesActive < gNumArrayElements) {
        while (gParticleArray[gLowestUnusedElement].lifeRemaining > 0)
            gLowestUnusedElement++;

        if (gLowestUnusedElement >= gNumArrayElements || gLowestUnusedElement < 0)
            return;

        gParticleArray[gLowestUnusedElement].lifeRemaining = lifeCount;
        gParticleArray[gLowestUnusedElement].color = color;
        gParticleArray[gLowestUnusedElement].horizSpeed = horizSpeed;
        gParticleArray[gLowestUnusedElement].vertSpeed = vertSpeed;
        gParticleArray[gLowestUnusedElement].horizLoc = horizLoc;
        gParticleArray[gLowestUnusedElement].vertLoc = vertLoc;
        gParticleArray[gLowestUnusedElement].oldHorizLoc = horizLoc;
        gParticleArray[gLowestUnusedElement].oldVertLoc = vertLoc;
        gParticleArray[gLowestUnusedElement].shade = shade;

        if (gLowestUnusedElement > gLastElementInUse)
            gLastElementInUse = gLowestUnusedElement;

        gNumberOfParticlesActive++;
        gLowestUnusedElement++;
    }
}

/* each living particle inside bounds: its place in the view, colour and level of 32 */
void ForEachDrawnParticle(const Rect &bounds, void (*visit)(void *ctx, int x, int y, int32_t color, int level),
                          void *ctx)
{
    Particle *particlePtr;
    int32_t index;
    int horizLoc;
    int vertLoc;
    int level;

    if (gParticlesInitialized != 1)
        return;
    for (index = 0, particlePtr = gParticleArray; index <= gLastElementInUse; index++, particlePtr++) {
        if (particlePtr->lifeRemaining <= 1)
            continue;
        horizLoc = (particlePtr->horizLoc >> 8) - scrollOffsetHoriz;
        vertLoc = (particlePtr->vertLoc >> 8) - scrollOffsetVert;
        if (horizLoc >= bounds.right || horizLoc <= bounds.left || vertLoc >= bounds.bottom || vertLoc <= bounds.top)
            continue;
        if ((int32_t)particlePtr->shade != 32)
            level = particlePtr->lifeRemaining > 32 ? 32 : particlePtr->lifeRemaining;
        else
            level = particlePtr->lifeRemaining > particlePtr->shade ? particlePtr->shade : particlePtr->lifeRemaining;
        visit(ctx, horizLoc, vertLoc, particlePtr->color, level);
    }
}

// FUNCTION: EVNOVA 0x0047c800
// MAC: _MoveParticles i386 0x000c60c5 ppc 0x000d2304
void MoveParticles(void)
{
    Particle *p;
    int i;
    int last;

    last = gLastElementInUse;

    if (!gParticlesInitialized)
        return;

    i = 0;
    p = gParticleArray;
    for (; i <= last; i++, p++) {
        if (i >= gNumArrayElements)
            SW_ASSERT_FAIL("SWParticles.c", 0x221, 0x221);
        if (p->lifeRemaining > 0) {
            p->lifeRemaining--;
            if (p->lifeRemaining == 0) {
                gNumberOfParticlesActive--;
                if (i < gLowestUnusedElement)
                    gLowestUnusedElement = i;
            } else {
                p->oldHorizLoc = p->horizLoc;
                p->oldVertLoc = p->vertLoc;
                p->horizLoc += p->horizSpeed;
                p->vertLoc += p->vertSpeed;
                p->vertSpeed += gGravity;
            }
        }
    }

    if (gNumberOfParticlesActive <= 0) {
        gLastElementInUse = 0;
    } else {
        while (gParticleArray[gLastElementInUse].lifeRemaining == 0 && gLastElementInUse > 0)
            gLastElementInUse--;
    }
}
