/* atfiletypes.c -- the file manager's extension to Mac type/creator map, copied into a handle. Notes: Research/notes/atfiletypes.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "macport.h"
#include "atfiletool.h"

#define kExt(a, b, c, d) (((ULong)(a) << 24) | ((ULong)(b) << 16) | \
                          ((c) << 8) | (d))

#define kTypeMapCount 19

// FUNCTION: EVNOVA 0x00509d87
void **_FT_FileTypesCreate(void)
{
    int32_t err;
    void **h;
    void *p;

    err = 0;
    h = 0;
    {
        FileTypeMap map[kTypeMapCount] = {           /* initialiser at 0x005796e0 */
            { 'TEXT', 'ttxt', kExt('t', 'x', 't', 0), 1 },
            { 'TEXT', 'ttxt', kExt('t', 'e', 'x', 't'), 0 },
            { 'TEXT', 'CWIE', kExt('c', 0, 0, 0), 0 },
            { 'TEXT', 'CWIE', kExt('c', 'p', 0, 0), 0 },
            { 'TEXT', 'CWIE', kExt('c', 'p', 'p', 0), 0 },
            { 'TEXT', 'CWIE', kExt('h', 0, 0, 0), 0 },
            { 'TEXT', 'CWIE', kExt('h', 'p', 0, 0), 0 },
            { 'TEXT', 'CWIE', kExt('h', 'p', 'p', 0), 0 },
            { 'PNGf', 'ogle', kExt('p', 'n', 'g', 0), 1 },
            { 'PNG ', 'ogle', kExt('p', 'n', 'g', 0), 0 },
            { 'GIFf', 'ogle', kExt('g', 'i', 'f', 0), 1 },
            { 'GIF ', 'ogle', kExt('g', 'i', 'f', 0), 0 },
            { 'JPEG', 'ogle', kExt('j', 'p', 'g', 0), 1 },
            { 'JPEG', 'ogle', kExt('j', 'p', 'e', 'g'), 0 },
            { 'BMPf', 'ogle', kExt('b', 'm', 'p', 0), 1 },
            { 'BMP ', 'ogle', kExt('b', 'm', 'p', 0), 0 },
            { 'PICT', 'ttxt', kExt('p', 'c', 't', 0), 1 },
            { 'PICT', 'ttxt', kExt('p', 'i', 'c', 't'), 0 },
            { '????', '????', 0, 0 }
        };

        h = IndirectAllocate(sizeof(map));
        if (h == 0)
            err = kOutOfMemory;
        else {
            p = *h;
            memmove(p, map, sizeof(map));
        }
    }
    if (err != 0) {
        IndirectDeallocate(h);
        h = 0;
    }
    return h;
}

#define kPreferred          1

#define ToLower(c)          ((c) + (((c) >= 'A' && (c) <= 'Z') ? 'a' - 'A' : 0))


ULong _FT_Suffix2Value(const char *name);

/* _FT_FileTypesLookup (the map entry for a name's extension, else for `type`; a preferred entry
   first, the '????' terminator when none). */
// FUNCTION: EVNOVA 0x00509ded
void _FT_FileTypesLookup(const char *name, ULong type, FileTypeMap *entry)
{
    int32_t err;
    ULong extension;
    ULong count;
    ULong i;

    count = 0;
    extension = 0;
    err = 0;
    if (name != 0 && !IsSpecID(name))
        _FT_PathLookup((FolderSpec **)name);
    IndirectGetSize((void **)gFT_FileTypes);
    extension = _FT_Suffix2Value(name);
    if (extension != 0) {
        count = IndirectGetSize((void **)gFT_FileTypes) / sizeof(FileTypeMap);
        for (i = 0; i < count; i++)
            if ((*gFT_FileTypes)[i].extension == extension && ((*gFT_FileTypes)[i].preferred & kPreferred))
                break;
        if (i == count)
            for (i = 0; i < count; i++)
                if ((*gFT_FileTypes)[i].extension == extension)
                    break;
        if (i == count)
            i = count - 1;
    } else {
        count = IndirectGetSize((void **)gFT_FileTypes) / sizeof(FileTypeMap);
        for (i = 0; i < count; i++)
            if ((*gFT_FileTypes)[i].type == type && ((*gFT_FileTypes)[i].preferred & kPreferred))
                break;
        if (i == count)
            for (i = 0; i < count; i++)
                if ((*gFT_FileTypes)[i].type == type)
                    break;
        if (i == count)
            i = count - 1;
    }
    *entry = (*gFT_FileTypes)[i];
}

/* _FT_Suffix2Value (a name's extension as a left-justified lower-case 4-char code; the whole
   name when it has no '.'). */
// FUNCTION: EVNOVA 0x00509f08
ULong _FT_Suffix2Value(const char *name)
{
    int32_t err;
    ULong code;
    const char *s;
    ULong i;

    err = 0;
    code = 0;
    s = 0;
    if (name != 0 && !IsSpecID(name))
        _FT_PathLookup((FolderSpec **)name);
    if (name != 0 && name[0] != 0) {
        s = name;
        while ((s = StringFindDelimiter(s, ".")) != 0) {
            name = s;
            s++;
        }
        if (*name == '.')
            name++;
        for (i = 0; i < 4 && name[i] != 0; i++)
            ((char *)&code)[3 - i] = ToLower(name[i]);
    }
    return code;
}
