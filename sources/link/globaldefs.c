/* globaldefs.c -- the data globals whose type and extent the bucket run has SETTLED, defined here so the tree carries them instead of a generated placeholder. Notes: Research/notes/globaldefs.md */
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "burger.h"
#include "burgerchan.h"
#include "crashlog.h"
#include "render.h"
#include "ships.h"
#include "game.h"
#include "dialogs.h"
#include "player.h"
#include "ai.h"
#include "gameutil.h"
#include "missions.h"
#include "stellar.h"
#include "cefix.h"

int gDisableUpdatesMode;
uint32_t ScreenWidth;
uint32_t ScreenHeight;
uint32_t VideoWidth;
SpriteRec *gSWCurrentSpriteBeingDrawn;
char gNeedUpdateFlag;
Word8 ResourceJustLoaded;
