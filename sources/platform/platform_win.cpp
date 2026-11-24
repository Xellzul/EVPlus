/* platform_win.cpp -- what of platform.h is still Windows only: opening a file and changing the current
   folder by a UTF-8 path, the program in the registry, and the installed fonts. Everything else is platform_sdl.cpp. */
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <wchar.h>
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

int PlatSetCurrentFolder(const char *path)
{
    wchar_t widePath[kWidePath];

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, widePath, kWidePath) == 0)
        return 0;
    return _wchdir(widePath) == 0;
}

/* -- the program in the registry, as CE wrote it at every start -------------------------------------- */

#define kKeyMax     512
#define kNameMax    128

static void SetRegValue(const wchar_t *key, const wchar_t *value)
{
    HKEY hkey;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL)
            != ERROR_SUCCESS)
        return;
    RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE *)value, (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));
    RegCloseKey(hkey);
}

/* `key' = prefix + name (+ suffix) */
static void MakeKey(wchar_t *key, const wchar_t *prefix, const char *name, const wchar_t *suffix)
{
    wchar_t wide[kNameMax];

    wide[0] = 0;
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, kNameMax);
    wide[kNameMax - 1] = 0;
    wcscpy(key, prefix);
    wcscat(key, wide);
    wcscat(key, suffix);
}

void PlatRegisterProgram(const char *programName)
{
    wchar_t exePath[kWidePath];
    wchar_t key[kKeyMax];

    if (GetModuleFileNameW(NULL, exePath, kWidePath) == 0)
        return;
    exePath[kWidePath - 1] = 0;
    MakeKey(key, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\", programName, L"");
    SetRegValue(key, exePath);
}

/* -- installed fonts ------------------------------------------------------------------------------- */

#define kTtcTag     0x66637474          /* 'ttcf': the whole collection file a font is part of */
#define kWholeFile  0
#define kProbeSize  12

void *PlatSystemFont(const char *family, int bold, int italic, int lookAlike, int *size)
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
        /* Windows answers any name with some font: only the one asked for counts, unless any will do */
        if (lookAlike || lstrcmpiA(got, family) == 0) {
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
