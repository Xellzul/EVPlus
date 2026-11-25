/* platform_unix.cpp -- what of platform.h differs on macOS, Linux and other Unix systems: opening a file and
   changing the current folder (names in any case), the program's file type, and the installed fonts (Core Text
   on macOS, fontconfig elsewhere). Everything else is platform_sdl.cpp. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/resource.h>
#include <unistd.h>
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#else
#include <fontconfig/fontconfig.h>
#endif
#include "platform.h"

FILE *PlatFileOpen(const char *name, const char *mode)
{
    char path[kPlatPathMax];

    if (!PlatFindPath(name, path, sizeof path))
        return NULL;
    return fopen(path, mode);
}

int PlatSetCurrentFolder(const char *name)
{
    char path[kPlatPathMax];

    if (!PlatFindPath(name, path, sizeof path))
        return 0;
    return chdir(path) == 0;
}

/* A desktop learns of programs from the .desktop files an installer puts in place; the program does not
   write them itself. */
void PlatRegisterProgram(const char *programName)
{
    (void)programName;
}

/* -- installed fonts ------------------------------------------------------------------------------- */

#ifdef __APPLE__
#define kTraitCount 1
#define kFallbackFamily "Helvetica"         /* what a look-alike is when Core Text knows no such name */

/* The installed font of that family with these traits (0 = any), as Core Text matches it */
static CTFontDescriptorRef MatchFont(CFStringRef name, uint32_t bits)
{
    CFMutableDictionaryRef wanted;
    CFMutableDictionaryRef traits;
    CFNumberRef traitBits;
    CTFontDescriptorRef asked;
    CTFontDescriptorRef found;

    wanted = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (wanted == NULL)
        return NULL;
    CFDictionarySetValue(wanted, kCTFontFamilyNameAttribute, name);
    if (bits != 0) {
        traits = CFDictionaryCreateMutable(NULL, kTraitCount, &kCFTypeDictionaryKeyCallBacks,
                                           &kCFTypeDictionaryValueCallBacks);
        traitBits = CFNumberCreate(NULL, kCFNumberSInt32Type, &bits);
        if (traits != NULL && traitBits != NULL) {
            CFDictionarySetValue(traits, kCTFontSymbolicTrait, traitBits);
            CFDictionarySetValue(wanted, kCTFontTraitsAttribute, traits);
        }
        if (traitBits != NULL)
            CFRelease(traitBits);
        if (traits != NULL)
            CFRelease(traits);
    }
    asked = CTFontDescriptorCreateWithAttributes(wanted);
    found = asked != NULL ? CTFontDescriptorCreateMatchingFontDescriptor(asked, NULL) : NULL;
    if (asked != NULL)
        CFRelease(asked);
    CFRelease(wanted);
    return found;
}

/* Core Text answers any name with some font: only one of the family asked for counts, unless any will do. A
   family without the bold or italic member answers with its plain one. */
void *PlatSystemFont(const char *family, int bold, int italic, int lookAlike, int *size)
{
    CFStringRef name;
    CTFontDescriptorRef found;
    CFStringRef foundFamily;
    CFURLRef url;
    char path[kPlatPathMax];
    uint32_t bits;
    void *data;

    *size = 0;
    data = NULL;
    name = CFStringCreateWithCString(NULL, family, kCFStringEncodingUTF8);
    if (name == NULL)
        return NULL;
    bits = (bold ? kCTFontBoldTrait : 0) | (italic ? kCTFontItalicTrait : 0);
    found = MatchFont(name, bits);
    if (found == NULL && bits != 0)
        found = MatchFont(name, 0);
    if (found == NULL && lookAlike)
        found = MatchFont(CFSTR(kFallbackFamily), bits);
    if (found != NULL) {
        foundFamily = (CFStringRef)CTFontDescriptorCopyAttribute(found, kCTFontFamilyNameAttribute);
        url = (CFURLRef)CTFontDescriptorCopyAttribute(found, kCTFontURLAttribute);
        if (url != NULL
                && (lookAlike || (foundFamily != NULL
                                  && CFStringCompare(foundFamily, name, kCFCompareCaseInsensitive)
                                     == kCFCompareEqualTo))
                && CFURLGetFileSystemRepresentation(url, true, (UInt8 *)path, sizeof path))
            data = PlatLoadFile(path, size);
        if (foundFamily != NULL)
            CFRelease(foundFamily);
        if (url != NULL)
            CFRelease(url);
        CFRelease(found);
    }
    CFRelease(name);
    return data;
}
#else
/* one of the font's family names is `family' */
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

/* fontconfig answers any name with some font: only one of the family asked for counts, unless any will do */
void *PlatSystemFont(const char *family, int bold, int italic, int lookAlike, int *size)
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
    FcPatternAddBool(wanted, FC_SCALABLE, FcTrue);          /* a bitmap-only font cannot be sized */
    FcConfigSubstitute(NULL, wanted, FcMatchPattern);
    FcDefaultSubstitute(wanted);
    font = FcFontMatch(NULL, wanted, &result);
    if (font != NULL) {
        if ((lookAlike || IsFamily(font, family)) && FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
            data = PlatLoadFile((const char *)file, size);
        FcPatternDestroy(font);
    }
    FcPatternDestroy(wanted);
    return data;
}
#endif

uint64_t PlatCpuMicroseconds(void)
{
    struct rusage use;

    if (getrusage(RUSAGE_SELF, &use) != 0)
        return 0;
    return (uint64_t)(use.ru_utime.tv_sec + use.ru_stime.tv_sec) * 1000000
           + use.ru_utime.tv_usec + use.ru_stime.tv_usec;
}
