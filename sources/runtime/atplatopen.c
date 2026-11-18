/* atplatopen.c -- the platform layer: open and close. The exe's layer also looks up four kernel calls
   that old Windows lacks, finds or makes a hidden window, installs console and crash handlers that close the
   layer, starts the thread table and a clock timer, keeps idle and quit callback tables, and builds
   AmbrosiaTools' table of text encodings (atcharset.c with its hash tables and Unicode helpers, which
   nothing else of the game used); none of it has a user left. Notes: Research/notes/atplatopen.md */

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"

typedef void (*PlatformCallback)(void);

unsigned char gPlatformOpen;                      /* 0x00870e10, 1 B */

// FUNCTION: EVNOVA 0x004d3163
int32_t PlatformOpen(void)
{
    gPlatformOpen = 1;
    return 0;
}

// FUNCTION: EVNOVA 0x004d332d
void PlatformClose(void)
{
    gPlatformOpen = 0;
}

// BUG: the exe (0x004d34a0) looks the proc up in its table of quit procs and never adds it, so the table
// stays empty and none of the procs handed in (FT_Close, ST_Close, _EncodingPostflight) runs when the layer
// closes or the program exits; the game closes its tools itself (engine/setup.c)
void SystemSetQuitProc(PlatformCallback fn)
{
    (void)fn;
}
