/* fontcache.cpp -- the port's font families: a Mac font name to the font file its text is drawn with. The exe
   kept a list of GDI fonts per family and size (CreateFontA; win 0x004bc3e0-0x004bc75d); the files are
   opened here and drawn by port/fontraster.cpp. Notes: Research/notes/fontcache.md */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "macport.h"
#include "dialogs.h"

#include "burgerplat.h"
#include "burger.h"
#include "cefix.h"
#include "platform.h"
#include "fontraster.h"

#define kStyleCount     4               /* plain, bold, italic, bold italic: kFontBold | kFontItalic */
#define kSourceMax      32

typedef struct FontFamily {
    char name[0x100];           /* +0x000 the Mac name the game asked for; GetFNum compares it */
    RasterFont *faces[kStyleCount];     /* the font file each style is drawn with */
    char looked[kStyleCount];           /* that style has been looked up (it may have found nothing) */
    char heavy;                 /* the font already has a bold's weight: bold is not added to it */
    char source[kSourceMax];    /* where the plain face came from, for the font test page */
} FontFamily;

/* a font file from beside the exe */
typedef struct GameFont {
    struct GameFont *next;
    RasterFont *font;
} GameFont;

short gChicagoFont;
short gTimesFont;
ListRec gFontFamilies;
static GameFont *gGameFonts;

/* CE drew its text through cnc-ddraw, which changes every font the game creates by two ddraw.ini settings;
   these are the values CE ships (min_font_size is kMinFontSize, macport.h): */
#define kAntiAliasedMinSize     13      /* anti_aliased_fonts_min_size: a smaller font is not smoothed */

#define kFontPattern    "*.ttf"

static FontFamily *AddFontFamily(const char *name);

static void AddGameFont(const char *path, void *ctx)
{
    GameFont *entry;
    RasterFont *font;
    void *data;
    int size;

    (void)ctx;
    data = PlatLoadFile(path, &size);
    font = RasterOpen(data, size, NULL);
    if (font == NULL)
        return;
    entry = (GameFont *)malloc(sizeof(GameFont));
    if (entry == NULL) {
        RasterClose(font);
        return;
    }
    entry->font = font;
    entry->next = gGameFonts;
    gGameFonts = entry;
}

/* The fonts beside the exe are the game's own: CE ships Geneva.ttf and Charcoal.ttf there and leaves them
   to be installed by hand; not installed, every Mac font name got Windows' default font. */
// FUNCTION: EVNOVA 0x004bc3e0
void InitFonts(void)
{
    short scratch;

    PlatListFiles(PlatExeFolder(), kFontPattern, AddGameFont, NULL);
    LinkedListInit(&gFontFamilies);
    GetFNum("\pChicago", &gChicagoFont);
    GetFNum("\pTimes", &gTimesFont);
    GetFNum("\pGeneva", &scratch);
}

/* A Mac font that is always drawn with a Windows font: the one entry CE kept of the exe's table
   (0x00575d80), which also had Helvetica, Geneva, Chicago and Charcoal as Arial, Courier as Courier New
   and New York as Georgia. */
typedef struct FontMap {
    const char *macName;
    const char *faceName;
} FontMap;

static const FontMap gFontMap[] = {
    { "Times",     "Times New Roman" }
};

/* What a Mac font is drawn with when there is no font of that name, neither beside the exe nor installed:
   the first of these that exists.
   - Chicago is the Mac's system font, font number 0; from Mac OS 8 on that was Charcoal.
     The port draws its dialogs in bold Chicago (dialogmgr.cpp) because the original game had Arial stand in
     for it and Arial needs the bold to have Chicago's weight; Charcoal has that weight as it is, so drawn
     with Charcoal a bold Chicago stays plain (`heavy'), as the Mac draws its dialogs.
   - The rest is the table of the original Windows game, which used it always; CE dropped it (all but
     Times, above) so that the Mac fonts show where they are installed. */
#define kMaxSubstitutes     2

typedef struct FontSubstitute {
    const char *macName;
    const char *faces[kMaxSubstitutes];
    char heavy[kMaxSubstitutes];
} FontSubstitute;

static const FontSubstitute kSubstitutes[] = {
    { "Chicago",   { "Charcoal", "Arial" }, { 1, 0 } },
    { "Geneva",    { "Arial", NULL } },
    { "Charcoal",  { "Arial", NULL } },
    { "Helvetica", { "Arial", NULL } },
    { "Courier",   { "Courier New", NULL } },
    { "New York",  { "Georgia", NULL } }
};

/* with none of them there: the font nearly all of the game's text is in */
#define kLastResort         "Geneva"

/* the font file of that family name: from beside the exe, the member closest to the style, else installed */
static RasterFont *FindFace(const char *name, int style, const char **source)
{
    GameFont *entry;
    RasterFont *best;
    void *data;
    int size;
    int miss;
    int bestMiss;

    best = NULL;
    bestMiss = kStyleCount;
    for (entry = gGameFonts; entry != NULL; entry = entry->next) {
        if (stricmp(RasterFamily(entry->font), name) != 0)
            continue;
        miss = (RasterIsBold(entry->font) != ((style & kFontBold) != 0))
               + (RasterIsItalic(entry->font) != ((style & kFontItalic) != 0));
        if (miss < bestMiss) {
            best = entry->font;
            bestMiss = miss;
        }
    }
    if (best != NULL) {
        *source = "beside the game";
        return best;
    }
    data = PlatSystemFont(name, (style & kFontBold) != 0, (style & kFontItalic) != 0, 0, &size);
    *source = "installed";
    return RasterOpen(data, size, name);
}

/* the family's font file for a style, looked up once */
static RasterFont *FamilyFace(FontFamily *family, int style)
{
    const char *source;
    RasterFont *font;
    unsigned i;
    int j;

    style &= kFontBold | kFontItalic;
    if (family->looked[style])
        return family->faces[style];
    family->looked[style] = 1;
    source = "";
    font = NULL;
    for (i = 0; i < sizeof(gFontMap) / sizeof(gFontMap[0]) && font == NULL; i++) {
        if (stricmp(family->name, gFontMap[i].macName) == 0)
            font = FindFace(gFontMap[i].faceName, style, &source);
    }
    if (font == NULL)
        font = FindFace(family->name, style, &source);
    for (i = 0; i < sizeof(kSubstitutes) / sizeof(kSubstitutes[0]) && font == NULL; i++) {
        if (stricmp(family->name, kSubstitutes[i].macName) != 0)
            continue;
        for (j = 0; j < kMaxSubstitutes && kSubstitutes[i].faces[j] != NULL && font == NULL; j++) {
            font = FindFace(kSubstitutes[i].faces[j], style, &source);
            if (font != NULL)
                family->heavy = kSubstitutes[i].heavy[j];
        }
    }
    if (font == NULL)
        font = FindFace(kLastResort, style, &source);
    if (font == NULL && gGameFonts != NULL) {
        font = gGameFonts->font;
        source = "beside the game";
    }
    if (font == NULL) {
        /* no font file of any of these names: what the system draws the family with */
        void *data;
        int size;

        data = PlatSystemFont(family->name, (style & kFontBold) != 0, (style & kFontItalic) != 0, 1, &size);
        font = RasterOpen(data, size, NULL);
        source = "installed look-alike";
    }
    family->faces[style] = font;
    if (style == 0)
        strcpy(family->source, source);
    return font;
}

// FUNCTION: EVNOVA 0x004bc5c0
static FontFamily *AddFontFamily(const char *name)
{
    FontFamily *family;

    family = (FontFamily *)AllocAPointerClear(sizeof(FontFamily));
    strcpy(family->name, name);
    LinkedListAddNewEntryEnd(&gFontFamilies, family);
    return family;
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

/* What text of this family, size and style is drawn with. 0 = no such font number, or no font file at
   all: the exe's GetFont (0x004bc670) answers NULL then and GDI keeps the font the port had before. */
int GetFont(short familyID, short size, short style, FontSpec *spec)
{
    FontFamily *family;

    family = GetFontFamily(familyID);
    if (family == NULL)
        return 0;
    FamilyFace(family, 0);                      /* the plain face says whether the family is heavy */
    spec->font = FamilyFace(family, style);
    if (spec->font == NULL)
        return 0;
    spec->pixelSize = size < kMinFontSize ? kMinFontSize : size;
    spec->flags = spec->pixelSize >= kAntiAliasedMinSize ? kRasterSmooth : 0;
    if ((style & kFontBold) != 0 && family->heavy == 0 && !RasterIsBold(spec->font))
        spec->flags |= kRasterBold;
    if ((style & kFontItalic) != 0 && !RasterIsItalic(spec->font))
        spec->flags |= kRasterItalic;
    return 1;
}

/* -- for the font test page (port/fonttest.cpp) -- */

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

/* the font that family is really drawn with, and where its file comes from */
void FontRealName(short familyID, short size, short style, char *out, int outSize)
{
    FontFamily *family;
    FontSpec spec;

    out[0] = 0;
    family = GetFontFamily(familyID);
    if (family == NULL || !GetFont(familyID, size, style, &spec)) {
        strncpy(out, "no font", outSize - 1);
        out[outSize - 1] = 0;
        return;
    }
    snprintf(out, outSize, "%s (%s)", RasterFamily(spec.font), family->source);
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
