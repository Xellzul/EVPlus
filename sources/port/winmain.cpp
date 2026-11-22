/* winmain.cpp -- the game's entry, the exe's WinMain (win 0x004d2a80). The program's own entry point is
   the platform layer's, which calls this. Notes: Research/notes/winmain.md */
#include "burger.h"
#include "game.h"
#include "platform.h"

#define kGameIconGroup      202
#define kNplayIconGroup     203

/* Registers the program and its .nplay files with the system (on Windows written on every start), then
   makes the first argument the current folder. */
static void Startup(void)
{
    PlatRegisterFileType("EV Nova.exe", ".nplay", "EVNova.Play.1", "Play EV Nova", kNplayIconGroup,
                         kGameIconGroup);

    if (PlatArgCount() > 1)
        PlatSetCurrentFolder(PlatArg(1));
}


// FUNCTION: EVNOVA 0x004d2a80
int GameMain(void)
{
    Startup();
    CreateGameWindow();
    PlatSetCurrentFolder(PlatExeFolder());
    SetDefaultPrefixs();
    atexit(ShutdownWinGame);
    OSCursorReset();
    gRezOpenPending = 1;
    if (EVOpenResourceFile("9:Nova.rez") != 0) {
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
