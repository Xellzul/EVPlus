/* platform_unix.cpp -- what of platform.h differs on macOS, Linux and other Unix systems: opening a file and
   changing the current folder (names in any case), the program's file type, and the installed fonts (Core Text
   on macOS, fontconfig elsewhere). Everything else is platform_sdl.cpp. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#else
#include <fontconfig/fontconfig.h>
#endif
#include "platform.h"

#define kPathMax    4096                /* bytes of a path, with its end */

FILE *PlatFileOpen(const char *name, const char *mode)
{
    char path[kPathMax];

    if (!PlatFindPath(name, path, sizeof path))
        return NULL;
    return fopen(path, mode);
}

int PlatSetCurrentFolder(const char *name)
{
    char path[kPathMax];

    if (!PlatFindPath(name, path, sizeof path))
        return 0;
    return chdir(path) == 0;
}

/* A desktop learns of programs and file types from .desktop and MIME files that an installer puts in place;
   the program does not write them itself. */
void PlatRegisterFileType(const char *programName, const char *extension, const char *typeId,
                          const char *description, int icon, int fallbackIcon)
{
    (void)programName;
    (void)extension;
    (void)typeId;
    (void)description;
    (void)icon;
    (void)fallbackIcon;
}

/* -- installed fonts ------------------------------------------------------------------------------- */

#ifdef __APPLE__
#define kTraitCount 1

/* Core Text answers any name with some font: only one of the family asked for counts */
void *PlatSystemFont(const char *family, int bold, int italic, int *size)
{
    CFStringRef name;
    CFMutableDictionaryRef wanted;
    CFMutableDictionaryRef traits;
    CFNumberRef traitBits;
    CTFontDescriptorRef asked;
    CTFontDescriptorRef found;
    CFStringRef foundFamily;
    CFURLRef url;
    char path[kPathMax];
    uint32_t bits;
    void *data;

    *size = 0;
    data = NULL;
    name = CFStringCreateWithCString(NULL, family, kCFStringEncodingUTF8);
    if (name == NULL)
        return NULL;
    wanted = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(wanted, kCTFontFamilyNameAttribute, name);
    bits = (bold ? kCTFontBoldTrait : 0) | (italic ? kCTFontItalicTrait : 0);
    if (bits != 0) {
        traits = CFDictionaryCreateMutable(NULL, kTraitCount, &kCFTypeDictionaryKeyCallBacks,
                                           &kCFTypeDictionaryValueCallBacks);
        traitBits = CFNumberCreate(NULL, kCFNumberSInt32Type, &bits);
        CFDictionarySetValue(traits, kCTFontSymbolicTrait, traitBits);
        CFDictionarySetValue(wanted, kCTFontTraitsAttribute, traits);
        CFRelease(traitBits);
        CFRelease(traits);
    }
    asked = CTFontDescriptorCreateWithAttributes(wanted);
    found = asked != NULL ? CTFontDescriptorCreateMatchingFontDescriptor(asked, NULL) : NULL;
    if (found != NULL) {
        foundFamily = (CFStringRef)CTFontDescriptorCopyAttribute(found, kCTFontFamilyNameAttribute);
        url = (CFURLRef)CTFontDescriptorCopyAttribute(found, kCTFontURLAttribute);
        if (foundFamily != NULL && url != NULL
                && CFStringCompare(foundFamily, name, kCFCompareCaseInsensitive) == kCFCompareEqualTo
                && CFURLGetFileSystemRepresentation(url, true, (UInt8 *)path, sizeof path))
            data = PlatLoadFile(path, size);
        if (foundFamily != NULL)
            CFRelease(foundFamily);
        if (url != NULL)
            CFRelease(url);
        CFRelease(found);
    }
    if (asked != NULL)
        CFRelease(asked);
    CFRelease(wanted);
    CFRelease(name);
    return data;
}
#else
/* fontconfig answers any name with some font: only one of the family asked for counts */
static int IsFamily(FcPattern *font, const char *family)
{
    FcChar8 *name;
    int i;

    for (i = 0; FcPatternGetString(font, FC_FAMILY, i, &name) == FcResultMatch; i++) {
        if (strcasecmp((const char *)name, family) == 0)
            return 1;
    }
    return 0;
}

void *PlatSystemFont(const char *family, int bold, int italic, int *size)
{
    FcPattern *wanted;
    FcPattern *font;
    FcResult result;
    FcChar8 *file;
    void *data;

    *size = 0;
    data = NULL;
    wanted = FcPatternCreate();
    if (wanted == NULL)
        return NULL;
    FcPatternAddString(wanted, FC_FAMILY, (const FcChar8 *)family);
    FcPatternAddInteger(wanted, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(wanted, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(NULL, wanted, FcMatchPattern);
    FcDefaultSubstitute(wanted);
    font = FcFontMatch(NULL, wanted, &result);
    if (font != NULL) {
        if (IsFamily(font, family) && FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
            data = PlatLoadFile((const char *)file, size);
        FcPatternDestroy(font);
    }
    FcPatternDestroy(wanted);
    return data;
}
#endif
