/* burgerfilewin.cpp -- BurgerLib FmWin95Code on the platform layer (platform.h): directory scans, native
   paths, the default prefixes. Native paths are UTF-8, in Windows' form ("X:\dir\file", "\\server\share\dir")
   on Windows; elsewhere "/dir/file", which BurgerLib holds as ":dir:file:" (its network-share form, the
   file system's top standing for the share). Notes: Research/notes/burgerfilewin.md */

#include <stdio.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kSearchOpen         1
#define kVolumeColon        ':'
#ifdef _WIN32
#define kPathSeparator      '\\'
#else
#define kPathSeparator      '/'
#endif
#define kPathBuffer         0x2000
#define kBackslash          '\\'
#define kColon              ':'
#define kLowerA             'a'
#define kAboveLowerZ        '{'
#define kUpperMask          0xdf
#define kDriveRoot          2                    /* "X:" */

/* DoesFileExistNative (BurgerLib FmWin95Code): 1 for a file, 0 for a directory or nothing. */
// FUNCTION: EVNOVA 0x004d9150
Word DoesFileExistNative(const char *name)
{
    return PlatPathKind(name, 0) == kPlatFile;
}

/* The exe walked the folder with FindFirstFile and FindNextFile; the platform layer hands over the whole
   listing at once, so it is kept and stepped through. Its order is the one Windows gives an NTFS folder,
   by name, on every system: plug-ins load in it. */
typedef struct FolderItem {
    struct FolderItem *next;
    Word32 FileSize;
    char Dir;
    char Name[1];
} FolderItem;

typedef struct Listing {
    FolderItem *head;
    FolderItem *tail;
    FolderItem *step;                            /* the entry GetADirectoryEntry gives next */
} Listing;

static void AddItem(const char *name, int isFolder, uint64_t size, void *ctx)
{
    Listing *listing = (Listing *)ctx;
    FolderItem *item;
    Word length;

    length = strlen(name);
    if (length >= sizeof(((DirectorySearch_t *)0)->Name))
        return;
    item = (FolderItem *)AllocAPointer(sizeof(FolderItem) + length);
    if (item == 0)
        return;
    item->next = 0;
    item->Dir = isFolder != 0;
    /* BUG: the exe read the size's two halves the wrong way round: a file is 0xffffffff bytes unless its
       length is a multiple of 4 GB. All the game asks is whether a file is empty. */
    if ((Word32)size != 0)
        item->FileSize = 0xffffffff;
    else
        item->FileSize = (Word32)(size >> 32);
    memcpy(item->Name, name, length + 1);
    if (listing->tail != 0)
        listing->tail->next = item;
    else
        listing->head = item;
    listing->tail = item;
}

static void FreeListing(Listing *listing)
{
    FolderItem *item;
    FolderItem *next;

    for (item = listing->head; item != 0; item = next) {
        next = item->next;
        DeallocAPointer(item);
    }
    DeallocAPointer(listing);
}

/* OpenADirectory (BurgerLib FmWin95Code). */
// FUNCTION: EVNOVA 0x004d9180
Word OpenADirectory(DirectorySearch_t *dir, const char *name)
{
    char path[kPathBufferSize];
    Listing *listing;

    ExpandAPathToBufferNative(path, name);
    listing = (Listing *)AllocAPointerClear(sizeof(Listing));
    if (listing != 0) {
        if (PlatListFolder(path, AddItem, listing)) {
            listing->step = listing->head;
            dir->FindHandle = listing;
            dir->HandleOk = kSearchOpen;
            return 0;
        }
        FreeListing(listing);
    }
    return 1;
}

/* GetADirectoryEntry (BurgerLib FmWin95Code): the next entry; 1 at the end. The exe skipped "." and
   "..", which the platform layer never lists, and also told hidden, system and read-only entries and
   their dates, which nothing read. */
// FUNCTION: EVNOVA 0x004d9280
Word GetADirectoryEntry(DirectorySearch_t *dir)
{
    Listing *listing;
    FolderItem *item;

    listing = (Listing *)dir->FindHandle;
    if (dir->HandleOk == 0 || listing->step == 0)
        return 1;
    item = listing->step;
    listing->step = item->next;
    strcpy(dir->Name, item->Name);
    dir->Dir = item->Dir;
    dir->FileSize = item->FileSize;
    return 0;
}

/* CloseADirectory (BurgerLib FmWin95Code). */
// FUNCTION: EVNOVA 0x004d94b0
void CloseADirectory(struct DirectorySearch_t *dir)
{
    if (dir->HandleOk != 0) {
        FreeListing((Listing *)dir->FindHandle);
        dir->HandleOk = 0;
        dir->FindHandle = 0;
    }
}

/* DeleteAFileNative (BurgerLib FmWin95Code): a file, or failing that an empty directory;
   answers 0 on success. */
// FUNCTION: EVNOVA 0x004d94f0
Word DeleteAFileNative(const char *name)
{
    if (PlatRemove(name))
        return 0;
    return 1;
}

/* Create every directory along a native path; 0 once the last exists. As in the exe, a file of that
   name counts as the directory being there. */
/* CreateDirectoryPathNative (BurgerLib CreateDirectoryPathNative). */
// FUNCTION: EVNOVA 0x004d9530
int32_t CreateDirectoryPathNative(char *path)
{
    if (PlatMakeFolder(path))
        return 0;
    return 1;
}

/* ExpandAPathToBufferNative (BurgerLib FmWin95Code): ".Dn:dir:file" becomes "X:\dir\file" for drive n,
   ":server:share:dir" becomes "\\server\share\dir". The exe first asked every drive from C to Z whether
   its volume label was `server' and, finding one, answered "X:\share\dir"; only a path of the second form
   ever got there (the game makes them from a network place it is run from, ConvertNativePathToPath), for
   which that drive would have been the wrong place. */
// FUNCTION: EVNOVA 0x004d9680
void ExpandAPathToBufferNative(char *out, const char *name)
{
    char path[kPathBufferSize];
    Word8 c;
    Word t;
    Word index;
    Word device;
    char *src;

    ExpandAPathToBuffer(path, name);
    device = kNoVolume;
    src = path;
    if ((Word8)path[0] == ':') {
#ifdef _WIN32
        *out++ = kPathSeparator;
#else
        ++src;                                  /* ":dir:" is "/dir" */
        *out++ = kPathSeparator;
#endif
    } else if ((Word8)src[0] == '.') {
        t = (Word8)src[1];
        if (t >= 'a' && t < 'z' + 1)
            t &= 0xdf;
        if (t == 'D') {
            index = 2;
            device = 0;
            for (;;) {
                t = (Word8)src[index];
                if (t == ':') {
                    ++index;
                    break;
                }
                if (t < '0' || t >= '9' + 1) {
                    index = 0;
                    device = kNoVolume;
                    break;
                }
                ++index;
                device = device * 10 + (t - '0');
            }
            src += index;
        }
    }
#ifdef _WIN32
    if (device != kNoVolume) {
        out[0] = (device + 'A');
        out[1] = ':';
        out[2] = kPathSeparator;
        out += 3;
    }
#endif
    if (*src != 0) {
        do {
            c = *src++;
            if (c == ':')
                c = kPathSeparator;
            *out++ = c;
        } while (c != 0);
        out -= 2;
        if ((Word8)*out != kPathSeparator)
            ++out;
    }
    *out = 0;
}

/* The length of an open file, its position left where it was. */
// FUNCTION: EVNOVA 0x004d9920
ULong fgetfilesize(FILE *fp)
{
    int32_t here;
    int32_t length;

    here = ftell(fp);
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, here, SEEK_SET);
    return length;
}

/* BurgerLib publishes PREFIXBOOT 32, PREFIXPREFS 33 and PREFIXSYSTEM 34, which are
   this file's 0x20, 0x21 and 0x22. */

#ifdef _WIN32
static Word PrefsFolder(char *folder, Word size)
{
    return PlatEnv("APPDATA", folder, size);
}
#elif defined(__APPLE__)
/* ~/Library/Preferences, where the Mac versions kept their preferences */
static Word PrefsFolder(char *folder, Word size)
{
    char home[kPathBufferSize];
    int length;

    if (!PlatEnv("HOME", home, sizeof(home)))
        return 0;
    length = snprintf(folder, size, "%s/Library/Preferences", home);
    return length > 0 && (Word)length < size && PlatMakeFolder(folder);
}
#else
#define kPrefsFolderName    "EV Nova"

/* $XDG_CONFIG_HOME/EV Nova, or ~/.config/EV Nova; made when it is not there */
static Word PrefsFolder(char *folder, Word size)
{
    char base[kPathBufferSize];
    int length;

    if (PlatEnv("XDG_CONFIG_HOME", base, sizeof(base)) && base[0] == '/')
        length = snprintf(folder, size, "%s/%s", base, kPrefsFolderName);
    else if (PlatEnv("HOME", base, sizeof(base)))
        length = snprintf(folder, size, "%s/.config/%s", base, kPrefsFolderName);
    else
        return 0;
    return length > 0 && (Word)length < size && PlatMakeFolder(folder);
}
#endif

/* The current folder (8), the program's (9) and the one for preferences ("@:": %APPDATA% on Windows,
   ~/Library/Preferences on macOS, the user's configuration folder elsewhere). The exe also
   set the system folder ("$:") and the drive it is on ("*:"), which no path of the game names, took the
   program's folder from its first argument, and without an APPDATA variable (Windows 95) kept preferences
   in the Windows folder: now "@:" is then left unset, which is the current folder. */
// FUNCTION: EVNOVA 0x004d9990
void SetDefaultPrefixs(void)
{
    char folder[kPathBufferSize];
    char *path;

    path = ConvertNativePathToPath("");
    if (path != 0) {
        SetAPrefix(kPrefixCurrent, path);
        DeallocAPointer(path);
    }

    path = ConvertNativePathToPath(PlatExeFolder());
    if (path != 0) {
        SetAPrefix(kPrefixApp, path);
        DeallocAPointer(path);
    }

    if (PrefsFolder(folder, sizeof(folder))) {
        path = ConvertNativePathToPath(folder);
        if (path != 0) {
            SetAPrefix(PREFIXPREFS, path);
            DeallocAPointer(path);
        }
    }
}

#ifdef _WIN32
static Word IsSeparator(char c)
{
    return c == '\\' || c == '/';
}

/* how much of a whole path names its drive or network share: "X:" or "\\server\share"; 0 for neither */
static Word RootLength(const char *path)
{
    Word n;
    Word parts;

    if (IsSeparator(path[0]) && IsSeparator(path[1])) {
        n = 2;
        for (parts = 0; parts < 2; parts++) {
            while (path[n] != 0 && !IsSeparator(path[n]))
                ++n;
            if (parts == 0 && path[n] != 0)
                ++n;
        }
        return n > 2 ? n : 0;                   /* two separators and no name are no share */
    }
    if (path[0] != 0 && path[1] == kVolumeColon)
        return kDriveRoot;
    return 0;
}

/* What the exe had GetFullPathName do: a path made whole against the current folder, "." and ".."
   resolved, '\' between its parts. `out' holds kPathBuffer bytes and gets "X:\..." or "\\server\share...". */
static void FullNativePath(char *out, const char *path)
{
    char whole[kPathBuffer];
    const char *part;
    Word root;
    Word floor;
    Word length;
    Word n;

    whole[0] = 0;
    if (RootLength(path) == 0) {
        PlatCurrentFolder(whole, kPathBuffer / 2);
        if (IsSeparator(path[0]))
            whole[RootLength(whole)] = 0;       /* from the current drive's top */
    }
    n = strlen(whole);
    length = strlen(path);
    if (length > kPathBuffer - 2 - n)          /* the result may gain a separator after "X:" */
        length = kPathBuffer - 2 - n;
    memcpy(whole + n, path, length);
    whole[n + length] = 0;

    root = RootLength(whole);
    if (root == 0) {
        out[0] = 0;                             /* no current folder to start from */
        return;
    }
    memcpy(out, whole, root);
    for (n = 0; n < root; n++) {
        if (out[n] == '/')
            out[n] = kPathSeparator;
    }
    floor = root;
    n = root;
    part = whole + root;
    while (*part != 0) {
        while (IsSeparator(*part))
            ++part;
        length = 0;
        while (part[length] != 0 && !IsSeparator(part[length]))
            ++length;
        if (length == 2 && part[0] == '.' && part[1] == '.') {
            while (n > floor && out[n - 1] != kPathSeparator)
                --n;
            if (n > floor)
                --n;
        } else if (length != 0 && !(length == 1 && part[0] == '.')) {
            out[n++] = kPathSeparator;
            memcpy(out + n, part, length);
            n += length;
        }
        part += length;
    }
    if (n == kDriveRoot && root == kDriveRoot)
        out[n++] = kPathSeparator;              /* a drive's top is "X:\" */
    out[n] = 0;
}

#else
/* A path made whole against the current folder, "." and ".." resolved: "/..." in `out' (kPathBuffer bytes),
   "" when there is no current folder to start from. */
static void FullNativePath(char *out, const char *path)
{
    char whole[kPathBuffer];
    const char *part;
    Word length;
    Word n;

    whole[0] = 0;
    if (path[0] != kPathSeparator)
        PlatCurrentFolder(whole, kPathBuffer / 2);
    n = strlen(whole);
    if (path[0] != kPathSeparator && (n == 0 || whole[0] != kPathSeparator)) {
        out[0] = 0;
        return;
    }
    length = strlen(path);
    if (length > kPathBuffer - 2 - n)
        length = kPathBuffer - 2 - n;
    memcpy(whole + n, path, length);
    whole[n + length] = 0;

    n = 0;
    part = whole;
    while (*part != 0) {
        while (*part == kPathSeparator)
            ++part;
        length = 0;
        while (part[length] != 0 && part[length] != kPathSeparator)
            ++length;
        if (length == 2 && part[0] == '.' && part[1] == '.') {
            while (n > 0 && out[n - 1] != kPathSeparator)
                --n;
            if (n > 0)
                --n;
        } else if (length != 0 && !(length == 1 && part[0] == '.')) {
            out[n++] = kPathSeparator;
            memcpy(out + n, part, length);
            n += length;
        }
        part += length;
    }
    if (n == 0)
        out[n++] = kPathSeparator;
    out[n] = 0;
}
#endif

/* ConvertNativePathToPath (BurgerLib ConvertNativePathToPath). */
// FUNCTION: EVNOVA 0x004d9b10
char *ConvertNativePathToPath(const char *path)
{
    char full[kPathBuffer];
    char out[kPathBuffer + 16];                  /* ".D25:" for "X:\", and a last ':' */
    char *dst;
    char *src;
    Word c;
    Word len;
    char *result;

    dst = out;
    src = full;
    if (*path == 0)
        path = ".";
    FullNativePath(src, path);
    if (src[0] == 0)
        return 0;
#ifdef _WIN32
    if (src[0] == kBackslash && src[1] == kBackslash) {
        *dst++ = kColon;
        src += 2;
    } else {
        c = src[0];
        if (c >= kLowerA && c < kAboveLowerZ)
            c &= kUpperMask;
        src += 3;
        dst[0] = '.';
        dst[1] = 'D';
        NumberToAscii(dst + 2, c - 'A', 0);
        dst += strlen(dst);
        *dst++ = kColon;
    }
#else
    *dst++ = kColon;                            /* "/dir" is ":dir:" */
    ++src;
#endif
    c = *src;
    while (c != 0) {
        if (c == kPathSeparator)
            c = kColon;
        *dst = c;
        ++src;
        c = *src;
        ++dst;
    }
    len = dst - out;
    if (len != 0 && out[len - 1] != kColon)
        out[len++] = kColon;

    result = (char *)AllocAPointer(len + 1);
    if (result != 0) {
        if (len != 0)
            memcpy(result, out, len);
        result[len] = 0;
    }
    return result;
}

#ifdef __cplusplus
}
#endif
