// COMPILE: -lang c++ -bool off
/* winmain.c -- the game's entry, the exe's WinMain (win 0x004d2a80). The program's own entry point is
   the platform layer's, which calls this. Notes: Research/notes/winmain.md */
#include <stddef.h>
#include <float.h>
#include "macport.h"
#include "burger.h"
#include "game.h"
#include "player.h"
#include "cefix.h"
#include "platform.h"

EXTERN_C_BEGIN


/* The exe also asked Windows how much memory the machine has (CE fixed the pool's size instead) and read
   three strings out of itself (LoadString 0x3b6..0x3b8, which its string table does not have), using
   none of it; and it did all of this only when no other copy of the game was running (burgeronce.c). */
// FUNCTION: EVNOVA 0x004d2a80
int GameMain(void)
{
    CE_Startup();
    CreateGameWindow();
    SetDefaultPrefixs();
    atexit(ShutdownWinGame);
    OSCursorReset();
    gRezOpenPending = 1;
    EVOpenResourceFile("9:Nova.rez");
    InitFonts();
    InitEVNova();
    return 0;
}

EXTERN_C_END
