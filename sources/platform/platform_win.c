/* platform_win.c -- what of platform.h is still Windows only: the path conversion. Everything else is
   platform_sdl.c. */
#include <windows.h>
#include "platform.h"

void PlatPathToUtf8(const char *path, char *out, int outSize)
{
    wchar_t wide[1024];

    out[0] = '\0';
    if (MultiByteToWideChar(CP_ACP, 0, path, -1, wide, 1024) == 0)
        return;
    if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, outSize, NULL, NULL) == 0)
        out[0] = '\0';
}
