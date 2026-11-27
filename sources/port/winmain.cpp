/* winmain.cpp -- the game's entry, the exe's WinMain (win 0x004d2a80). The program's own entry point is
   the platform layer's, which calls this. Notes: Research/notes/winmain.md */
#include "burger.h"
#include "game.h"
#include "platform.h"
#include "gamepaths.h"

/* Registers the program with the system (on Windows written on every start), then makes the first
   argument the current folder. */
static void Startup(void)
{
    PlatRegisterProgram("EV Nova.exe");

    if (PlatArgCount() > 1)
        PlatSetCurrentFolder(PlatArg(1));
}


// FUNCTION: EVNOVA 0x004d2a80
int GameMain(void)
{
    Startup();
    CreateGameWindow();
    PlatSetCurrentFolder(PlatExeFolder());
    InitPaths();
    atexit(ShutdownWinGame);
    OSCursorReset();
    if (EVOpenResourceFile(JoinPath(paths.exe, "Nova.rez").c_str()) != 0) {
        char message[kPlatPathMax + 256];

        snprintf(message, sizeof message, "Nova.rez was not found in %s\n\nEV Nova Plus needs the files of the "
                 "Windows (Community Edition) version of EV Nova in that folder: Nova.rez, Nova Files and "
                 "Nova Plug-ins.", PlatExeFolder());
        PlatAlert("EV Nova", message);
        ExitToShell();
    }
    InitFonts();
    InitEVNova();
    return 0;
}
