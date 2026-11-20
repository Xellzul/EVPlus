/* globaldefs.cpp -- the data globals whose type and extent the bucket run has SETTLED, defined here so the tree carries them instead of a generated placeholder. Notes: Research/notes/globaldefs.md */
#include "burger.h"
#include "stellar.h"

int gDisableUpdatesMode;
uint32_t ScreenWidth;
uint32_t ScreenHeight;
uint32_t VideoWidth;
SpriteRec *gSWCurrentSpriteBeingDrawn;
char gNeedUpdateFlag;
Word8 ResourceJustLoaded;
