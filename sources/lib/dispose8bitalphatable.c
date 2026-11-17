// COMPILE: -lang c++ -bool off
/* dispose8bitalphatable.c -- Dispose8BitAlphaTable (win 0x00474300). Notes: Research/notes/dispose8bitalphatable.md */
/* Frees the 5 x 256 array of pointers Construct8BitAlphaTable (0x004740d0) builds: the
   mac's `_alphaTable'. Both loop counters are SHORTS (`cmp bp, 0x100' and `cmp si, 5',
   both signed) while the row base is mwcc's own strength-reduced induction variable. */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"
#include "game.h"

uint32_t alpha;
uint32_t alphaR;
uint32_t alphaG;
uint32_t alphaB;
uint32_t overR;
uint32_t overG;
uint32_t overB;
uint32_t overBias;
short scaleR;
short scaleG;
short scaleB;
uint32_t biasColor;
unsigned char *alphaTable[5][256];

// FUNCTION: EVNOVA 0x00474300
// MAC: _Dispose8BitAlphaTable i386 0x000c1cb6 ppc 0x000cde30
void Dispose8BitAlphaTable(void)
{
    short i, j;

    for (i = 0; i < 5; i++)
        for (j = 0; j < 256; j++)
            DeallocAPointer(alphaTable[i][j]);
}
