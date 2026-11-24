/* gamepaths.cpp -- where the game's files are (gamepaths.h): the program's, preferences and game folders,
   and joining paths */
#include <stdio.h>
#include "gamepaths.h"
#include "macport.h"
#include "platform.h"

#ifdef _WIN32
#define kSeparator '\\'
#else
#define kSeparator '/'
#endif

GamePaths paths;

static bool IsSeparator(char c)
{
#ifdef _WIN32
    return c == '\\' || c == '/';
#else
    return c == '/';
#endif
}

#ifdef _WIN32
static int PrefsFolder(char *folder, int size)
{
    return PlatEnv("APPDATA", folder, size);
}
#elif defined(__APPLE__)
/* ~/Library/Preferences */
static int PrefsFolder(char *folder, int size)
{
    char home[kPlatPathMax];
    int length;

    if (!PlatEnv("HOME", home, sizeof(home)))
        return 0;
    length = snprintf(folder, size, "%s/Library/Preferences", home);
    return length > 0 && length < size && PlatMakeFolder(folder);
}
#else
#define kPrefsFolderName    "EV Nova"

/* $XDG_CONFIG_HOME/EV Nova, or ~/.config/EV Nova; made when it is not there */
static int PrefsFolder(char *folder, int size)
{
    char base[kPlatPathMax];
    int length;

    if (PlatEnv("XDG_CONFIG_HOME", base, sizeof(base)) && base[0] == '/')
        length = snprintf(folder, size, "%s/%s", base, kPrefsFolderName);
    else if (PlatEnv("HOME", base, sizeof(base)))
        length = snprintf(folder, size, "%s/.config/%s", base, kPrefsFolderName);
    else
        return 0;
    return length > 0 && length < size && PlatMakeFolder(folder);
}
#endif

void InitPaths()
{
    char folder[kPlatPathMax];

    paths.exe = PlatExeFolder();
    paths.prefs.clear();
    if (PrefsFolder(folder, sizeof(folder)))
        paths.prefs = folder;
    SetGameRoot(paths.exe);
}

void SetGameRoot(const std::string &root)
{
    paths.root = root;
    paths.data = JoinPath(root, "Nova Files");
    paths.plugins = JoinPath(root, "Nova Plug-Ins");
}

std::string JoinPath(const std::string &folder, const char *name)
{
    std::string path = folder;

    if (!path.empty() && !IsSeparator(path.back()))
        path += kSeparator;
    path += name;
    return path;
}

const char *PathTail(const char *path)
{
    const char *tail = path;

    for (; *path != 0; path++) {
        if (IsSeparator(*path))
            tail = path + 1;
    }
    return tail;
}

std::string PilotFilePath(const unsigned char *pname)
{
    char name[256];
    char utf8[0x400];

    PStr2CStr(name, pname);
    TextToUtf8(utf8, name, sizeof(utf8));
    return JoinPath(paths.pilots, utf8) + ".plt";
}
