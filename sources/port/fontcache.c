// COMPILE: -lang c++ -bool off
/* fontcache.c -- the port's font family/size cache (win 0x004bc600-0x004bc75d). Notes: Research/notes/fontcache.md */
#include <stddef.h>

#include "macport.h"
#include "dialogs.h"

#include "burgerplat.h"
#include "burger.h"
#include "win32.h"
#include "cefix.h"

/* one realised GDI font */
typedef struct FontEntry {
    short size;                 /* +0x00 */
    short style;                /* +0x02 */
    void *hfont;                /* +0x04 */
} FontEntry;

typedef struct FontFamily {
    char name[0x100];           /* +0x000 */
    ListRec fonts;              /* +0x100 */
    const char *face;           /* the Windows font it is drawn with; NULL until first needed */
} FontFamily;

short gChicagoFont;
short gTimesFont;
ListRec gFontFamilies;

/* the Mac-to-Windows substitution table the port ships */
typedef struct FontMap {
    const char *macName;        /* +0x00 */
    const char *faceName;       /* +0x04 */
    int charSet;                /* +0x08 */
    int boldWeight;             /* +0x0c */
    int normalWeight;           /* +0x10 */
    int pitchAndFamily;         /* +0x14 */
    char useDeviceCaps;         /* +0x18 */
} FontMap;

#define kFontMapCount 1
#define OUT_TT_PRECIS 5
#define NONANTIALIASED_QUALITY 3

/* CE drew its text through cnc-ddraw, which changes every font the game creates by two ddraw.ini settings;
   these are the values CE ships: */
#define kMinFontSize            10      /* min_font_size: a smaller font is made this size */
#define kAntiAliasedMinSize     13      /* anti_aliased_fonts_min_size: a smaller font is not smoothed */


static FontEntry *MakeFontEntry(FontFamily *family, int size, int style);

static FontFamily *AddFontFamily(const char *name);

#define FR_PRIVATE      0x10            /* for this process only, gone when it ends */
#define kExePathMax     260
#define kFontPattern    "*.ttf"

extern "C" __declspec(dllimport) int __stdcall AddFontResourceExA(const char *file, uint32_t flags,
                                                                 void *reserved);

/* The fonts beside the exe are made known to Windows for this run of the game. CE ships Geneva.ttf and
   Charcoal.ttf there and leaves them to be installed by hand; not installed, every Mac font name gets
   Windows' default font. */
static void LoadGameFolderFonts(void)
{
    char path[kExePathMax + sizeof(((WIN32_FIND_DATAA *)0)->cFileName)];
    WIN32_FIND_DATAA found;
    char *name;
    void *find;

    if (GetModuleFileNameA(NULL, path, kExePathMax) == 0)
        return;
    name = strrchr(path, '\\');
    name = (name != NULL) ? name + 1 : path;
    strcpy(name, kFontPattern);
    find = FindFirstFileA(path, &found);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do {
        strcpy(name, found.cFileName);
        AddFontResourceExA(path, FR_PRIVATE, NULL);
    } while (FindNextFileA(find, &found) != 0);
    FindClose(find);
}

// FUNCTION: EVNOVA 0x004bc3e0
void InitFonts(void)
{
    short scratch;

    LoadGameFolderFonts();
    LinkedListInit(&gFontFamilies);
    GetFNum("\pChicago", &gChicagoFont);
    GetFNum("\pTimes", &gTimesFont);
    GetFNum("\pGeneva", &scratch);
}

FontMap gFontMap[kFontMapCount] = {                      /* 0x00575d80 (name invented) */
    { "Times",     "Times New Roman", 0, FW_BOLD, FW_NORMAL, 0, 0 }
};

extern "C" __declspec(dllimport) int __stdcall GetTextFaceA(void *hdc, int count, char *name);

/* What a Mac font is drawn with when Windows has no font of that name (CE's Geneva.ttf and Charcoal.ttf
   are loaded from beside the exe; Chicago is not shipped): the first of these that exists.
   - Chicago is the Mac's system font, font number 0; from Mac OS 8 on that was Charcoal.
   - The rest is the table of the original Windows game, which used it always; CE dropped it (all but
     Times, above) so that the Mac fonts show where they are installed. */
#define kMaxSubstitutes     2
#define kFaceNameMax        64
#define kProbeHeight        12

typedef struct FontSubstitute {
    const char *macName;
    const char *faces[kMaxSubstitutes];
} FontSubstitute;

static const FontSubstitute kSubstitutes[] = {
    { "Chicago",   { "Charcoal", "Arial" } },
    { "Geneva",    { "Arial", NULL } },
    { "Charcoal",  { "Arial", NULL } },
    { "Helvetica", { "Arial", NULL } },
    { "Courier",   { "Courier New", NULL } },
    { "New York",  { "Georgia", NULL } }
};

/* does Windows draw a font asked for by this name with a font of this name? */
static Boolean FaceExists(const char *name)
{
    char got[kFaceNameMax];
    void *hdc;
    void *font;
    Boolean exists;

    exists = 0;
    hdc = CreateCompatibleDC(NULL);
    font = CreateFontA(-kProbeHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, OUT_TT_PRECIS, 0, 0, 0, name);
    if (hdc != NULL && font != NULL) {
        got[0] = 0;
        SelectObject(hdc, font);
        GetTextFaceA(hdc, sizeof(got), got);
        exists = stricmp(got, name) == 0;
    }
    if (hdc != NULL)
        DeleteDC(hdc);
    if (font != NULL)
        DeleteObject(font);
    return exists;
}

/* the family's own name where such a font exists, else its first substitute that does; with none of
   them there the name stays and Windows picks, as before */
static const char *FamilyFace(FontFamily *family)
{
    unsigned i;
    int j;

    if (family->face != NULL)
        return family->face;
    family->face = family->name;
    if (FaceExists(family->name))
        return family->face;
    for (i = 0; i < sizeof(kSubstitutes) / sizeof(kSubstitutes[0]); i++) {
        if (stricmp(family->name, kSubstitutes[i].macName) != 0)
            continue;
        for (j = 0; j < kMaxSubstitutes && kSubstitutes[i].faces[j] != NULL; j++) {
            if (FaceExists(kSubstitutes[i].faces[j])) {
                family->face = kSubstitutes[i].faces[j];
                break;
            }
        }
        break;
    }
    return family->face;
}

// FUNCTION: EVNOVA 0x004bc450
static FontEntry *MakeFontEntry(FontFamily *family, int size, int style)
{
    const char *faceName;
    int charSet;
    char useDeviceCaps;
    int pitchAndFamily;
    FontEntry *entry;
    int i;
    int weight;
    int height;
    int italic;
    int quality;
    void *hdc;

    useDeviceCaps = 0;
    for (i = 0; i < kFontMapCount; i++) {
        if (stricmp(family->name, gFontMap[i].macName) == 0) {
            if ((style & kFontBold) != 0)
                weight = gFontMap[i].boldWeight;
            else
                weight = gFontMap[i].normalWeight;
            charSet = gFontMap[i].charSet;
            pitchAndFamily = gFontMap[i].pitchAndFamily;
            faceName = gFontMap[i].faceName;
            useDeviceCaps = gFontMap[i].useDeviceCaps;
            break;
        }
    }
    if (i == kFontMapCount) {
        if ((style & kFontBold) != 0)
            weight = FW_BOLD;
        else
            weight = FW_NORMAL;
        charSet = 0;
        pitchAndFamily = 0;
        faceName = FamilyFace(family);
    }
    if (useDeviceCaps != 0) {
        hdc = GetDC(Win95MainWindow);
        height = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(Win95MainWindow, hdc);
    } else {
        height = -size;
    }
    if (height > -kMinFontSize)
        height = -kMinFontSize;
    quality = 0;
    if (-height < kAntiAliasedMinSize)
        quality = NONANTIALIASED_QUALITY;
    entry = (FontEntry *)AllocAPointerClear(sizeof(FontEntry));
    entry->size = size;
    entry->style = style;
    if ((style & kFontItalic) != 0)
        italic = 1;
    else
        italic = 0;
    entry->hfont = CreateFontA(height, 0, 0, 0, weight, italic, 0, 0, charSet,
                               OUT_TT_PRECIS, 0, quality, pitchAndFamily, faceName);
    LinkedListAddNewEntryEnd(&family->fonts, entry);
    return entry;
}

// FUNCTION: EVNOVA 0x004bc5c0
static FontFamily *AddFontFamily(const char *name)
{
    FontFamily *family;

    family = (FontFamily *)AllocAPointer(sizeof(FontFamily));
    strcpy(family->name, name);
    family->face = NULL;
    LinkedListInit(&family->fonts);
    LinkedListAddNewEntryEnd(&gFontFamilies, family);
    return family;
}

// FUNCTION: EVNOVA 0x004bc600
FontEntry *FindFontEntry(FontFamily *family, short size, short style)
{
    ListNode *node;
    FontEntry *entry;

    node = family->fonts.head;
    while (node != NULL) {
        entry = (FontEntry *)node->data;
        if (entry->size == size && entry->style == style)
            return entry;
        node = node->next;
    }
    return NULL;
}

// FUNCTION: EVNOVA 0x004bc640
FontFamily *GetFontFamily(short familyID)
{
    ListNode *node;
    FontFamily *family;

    node = (ListNode *)LinkedListGetEntryData(&gFontFamilies, familyID);
    if (node == NULL)
        family = NULL;
    else
        family = (FontFamily *)node->data;
    return family;
}

// FUNCTION: EVNOVA 0x004bc670
void *GetFont(short familyID, short size, short style)
{
    FontFamily *family;
    FontEntry *entry;

    family = GetFontFamily(familyID);
    if (family == NULL)
        return NULL;
    entry = FindFontEntry(family, size, style);
    if (entry == NULL)
        entry = MakeFontEntry(family, size, style);
    if (entry == NULL)
        return NULL;
    return entry->hfont;
}

/* -- for the font test page (port/fonttest.c) -- */


short FontFamilyCount(void)
{
    return gFontFamilies.count;
}

/* the name the game asked for, NULL past the last family */
const char *FontFamilyName(short familyID)
{
    FontFamily *family;

    family = GetFontFamily(familyID);
    return family != NULL ? family->name : NULL;
}

/* the name of the font Windows really draws that family with at this size and style */
void FontRealName(short familyID, short size, short style, char *out, int outSize)
{
    void *hdc;
    void *font;

    out[0] = 0;
    font = GetFont(familyID, size, style);
    hdc = CreateCompatibleDC(NULL);
    if (font != NULL && hdc != NULL) {
        SelectObject(hdc, font);
        GetTextFaceA(hdc, outSize, out);
    }
    if (hdc != NULL)
        DeleteDC(hdc);
}

// FUNCTION: EVNOVA 0x004bc6e0
void GetFNum(const unsigned char *name, short *familyID)
{
    ListNode *node;
    int index;
    char cname[256];

    index = 0;
    PStr2CStr(cname, name);
    node = gFontFamilies.head;
    while (node != NULL) {
        if (strcmp((char *)node->data, cname) == 0) {
            *familyID = index;
            return;
        }
        node = node->next;
        index = index + 1;
    }
    if (AddFontFamily(cname) != NULL)
        *familyID = (gFontFamilies.count - 1);
}
