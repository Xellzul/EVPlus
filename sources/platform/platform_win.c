/* platform_win.c -- what of platform.h is still Windows only: opening a file by a UTF-8 path and the
   installed fonts. Everything else is platform_sdl.c. */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "platform.h"

#define kWidePath   0x800               /* the game's longest path, in characters */
#define kWideMode   8

/* Windows' fopen takes the path in the system's code page, which cannot hold every file name: the wide
   one takes them all. */
FILE *PlatFileOpen(const char *path, const char *mode)
{
    wchar_t widePath[kWidePath];
    wchar_t wideMode[kWideMode];

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, widePath, kWidePath) == 0)
        return NULL;
    if (MultiByteToWideChar(CP_UTF8, 0, mode, -1, wideMode, kWideMode) == 0)
        return NULL;
    return _wfopen(widePath, wideMode);
}

#define kTtcTag     0x66637474          /* 'ttcf': the whole collection file a font is part of */
#define kWholeFile  0
#define kProbeSize  12

void *PlatSystemFont(const char *family, int bold, int italic, int *size)
{
    char got[LF_FACESIZE];
    HDC dc;
    HFONT font;
    HGDIOBJ old;
    DWORD length;
    DWORD table;
    void *data;

    *size = 0;
    data = NULL;
    dc = CreateCompatibleDC(NULL);
    font = CreateFontA(-kProbeSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic != 0, 0, 0, DEFAULT_CHARSET,
                       OUT_TT_PRECIS, 0, 0, 0, family);
    if (dc != NULL && font != NULL) {
        old = SelectObject(dc, font);
        got[0] = 0;
        GetTextFaceA(dc, sizeof got, got);
        /* Windows answers any name with some font: only the one asked for counts */
        if (lstrcmpiA(got, family) == 0) {
            table = kTtcTag;
            length = GetFontData(dc, table, 0, NULL, 0);
            if (length == GDI_ERROR) {
                table = kWholeFile;
                length = GetFontData(dc, table, 0, NULL, 0);
            }
            if (length != GDI_ERROR && length != 0) {
                data = malloc(length);
                if (data != NULL && GetFontData(dc, table, 0, data, length) == length) {
                    *size = (int)length;
                } else {
                    free(data);
                    data = NULL;
                }
            }
        }
        SelectObject(dc, old);
    }
    if (font != NULL)
        DeleteObject(font);
    if (dc != NULL)
        DeleteDC(dc);
    return data;
}
