// COMPILE: -lang c++ -bool off
/* winmain.c -- the port's WinMain (win 0x004d2a80). Notes: Research/notes/winmain.md */
#include <stddef.h>
#include <float.h>
#include "macport.h"
#include "burger.h"
#include "game.h"
#include "player.h"
#include "cefix.h"

EXTERN_C_BEGIN


// FUNCTION: EVNOVA 0x004d2a80
int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *lpCmdLine,
                      int nCmdShow)
{
    MemoryStatus ms;
    char buf[256];

    if (CE_Startup(lpCmdLine, nCmdShow) != 0)
        return 0;
    if (Win95Allow1Instance("Lock for EV Nova") == 0) {
        CreateGameWindow(hInstance, nCmdShow);
        ms.dwLength = 0x20;
        GlobalMemoryStatus(&ms);
        MaxMemSize = kCEMemoryPoolSize;
        if (MaxMemSize < 0x8000000)
            MaxMemSize = 0x8000000;
        InitMemory();
        SetDefaultPrefixs();
        atexit(ShutdownWinGame);
        OSCursorReset();
        InitializeQTML(2);
        gRezOpenPending = 1;
        EVOpenResourceFile("9:Nova.rez");
        SetLastError(0);
        LoadStringA(hInstance, 0x3b6, buf, 0x100);
        GetLastError();
        LoadStringA(hInstance, 0x3b7, buf, 0x100);
        GetLastError();
        LoadStringA(hInstance, 0x3b8, buf, 0x100);
        GetLastError();
        CE_InitScaleFactor();
        InitFonts();
        InitEVNova();
        TerminateQTML();
    }
    return 0;
}

EXTERN_C_END
