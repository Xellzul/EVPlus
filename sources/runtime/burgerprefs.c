// COMPILE: -lang c++ -bool off
/* burgerprefs.c -- the preferences file: open, close, write, lookup. Notes: Research/notes/burgerprefs.md */

#include <stddef.h>
#include <stdio.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The INI parser's arities reconcile against the vendor archive:
   @PrefFileNewFromMemory@8, @PrefFileNewFromFile@4 = OpenPrefsFile,
   @PrefFileSectionGetString@20 = PrefsGetString, and so on down the surface. */


/* The file is a circular list of lines; the file record itself (type 0) is the sentinel. */
typedef struct PrefsNode {
    int32_t Type;                                   /* +0x00 */
    struct PrefsNode *Next;                      /* +0x04 */
    struct PrefsNode *Prev;                      /* +0x08 */
    char *ValueText;                             /* +0x0c, type 2 only */
} PrefsNode;

#define kNodeFile    0
#define kNodeGroup   1
#define kNodeKey     2
#define kNodeText    3
/* the offsets and sizes, as on 32-bit (0x0c, 0x10, 0x0c, 0x10, 0x14), from the record's own layout */
#define kGroupName   offsetof(PrefsNode, ValueText)   /* inline, types 1 and 3 */
#define kKeyName     sizeof(PrefsNode)                /* inline, type 2 */
#define kLineHeader  offsetof(PrefsNode, ValueText)   /* the record without its text */
#define kTextLineSize (kGroupName + 4)                /* a section or verbatim line, text[4] */
#define kKeyLineSize (kKeyName + 4)                   /* an entry, value pointer and key[4] */
#define kLineSize    0x400

#define kOpenBracket  '['
#define kCloseBracket ']'
#define kEquals       '='
#define kSemicolon    ';'
#define kSlash        '/'

/* BurgerLib's PrefFileNew: an empty file, its record the list's sentinel. */
static PrefsNode *PrefFileNew(void)
{
    PrefsNode *file;

    file = (PrefsNode *)AllocAPointer(kLineHeader);
    if (file != 0) {
        file->Type = kNodeFile;
        file->Next = file;
        file->Prev = file;
    }
    return file;
}

/* BurgerLib's LineEntryAddComment: a verbatim line after last. */
static void LineEntryAddComment(PrefsNode *last, const char *text)
{
    PrefsNode *node;
    Word length;

    length = strlen(text);
    node = (PrefsNode *)AllocAPointer(length + kTextLineSize);
    if (node != 0) {
        memcpy((char *)node + kGroupName, text, length + 1);
        node->Type = kNodeText;
        node->Next = last->Next;
        node->Prev = last;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
}

extern PrefsNode *PrefFileAddSection(PrefsNode *file, const char *name);
extern void PrefFileSectionAddEntry(PrefsNode *group, const char *key, const char *value);

/* PrefFileNewFromMemory (BurgerLib PgPrefFile): parse an INI image line by line into
   sections, "key = value" entries and verbatim lines. */
// FUNCTION: EVNOVA 0x004ffa00
void *PrefFileNewFromMemory(const char *data, uint32_t length)
{
    char line[kLineSize];
    ULong used;
    PrefsNode *file;
    char *mark;
    Word c;

    file = PrefFileNew();
    if (file != 0) {
        if (length != 0) do {
            used = length;
            ExtractAString2(data, &used, kExtractStrDelimitLF, line, sizeof(line));
            length -= used;
            data += used;
            c = (Word8)line[0];
            if (c == kOpenBracket) {
                mark = strchr(line, kCloseBracket);
                if (mark != 0) {
                    *mark = 0;
                    PrefFileAddSection(file, line + 1);
                    continue;
                }
            }
            if (c != kEquals && c != kSemicolon && (c != kSlash || (Word8)line[1] != kSlash)) {
                mark = strchr(line, kEquals);
                if (mark != 0) {
                    *mark = 0;
                    StripLeadingAndTrailingWhiteSpace(line);
                    StripLeadingAndTrailingWhiteSpace(mark + 1);
                    if (line[0] != 0)
                        PrefFileSectionAddEntry(file->Prev, line, mark + 1);
                    continue;
                }
            }
            LineEntryAddComment(file->Prev, line);
        } while (length != 0);
        return file;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004ffc50
void *OpenPrefsFile(const char *path)
{
    void *text;
    uint32_t size;
    void *file;

    text = LoadAFile(path, &size);
    if (text == 0)
        return 0;
    file = PrefFileNewFromMemory((const char *)text, size);
    DeallocAPointer(text);
    return file;
}

// FUNCTION: EVNOVA 0x004ffcc0
void ClosePrefsFile(PrefsNode *file)
{
    PrefsNode *node;
    PrefsNode *next;

    if (file != 0) {
        node = file->Next;
        while (node != file) {
            switch (node->Type) {
            case kNodeKey:
                DeallocAPointer(node->ValueText);
                break;
            }
            next = node->Next;
            DeallocAPointer(node);
            node = next;
        }
        DeallocAPointer(file);
    }
}

// FUNCTION: EVNOVA 0x004ffeb0
PrefsNode *PrefsFindGroup(PrefsNode *file, const char *name)
{
    PrefsNode *node;

    node = file->Next;
    while (node != file) {
        if (node->Type == kNodeGroup
            && stricmp(((char *)node + kGroupName), name) == 0)
            return node;
        node = node->Next;
    }
    return 0;
}

/* PrefFileAddSection (BurgerLib PgPrefFile): a "[name]" line at the end of the file. */
// FUNCTION: EVNOVA 0x004fff10
PrefsNode *PrefFileAddSection(PrefsNode *file, const char *name)
{
    PrefsNode *node;
    Word length;

    length = strlen(name);
    node = (PrefsNode *)AllocAPointer(length + kTextLineSize);
    if (node != 0) {
        memcpy((char *)node + kGroupName, name, length + 1);
        node->Type = kNodeGroup;
        node->Next = file;
        node->Prev = file->Prev;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
    return node;
}

/* BurgerLib's PrefFileSectionFindEntry; the exe has no copy of its own, only this one inlined. */
static PrefsNode *PrefsFindEntry(PrefsNode *group, const char *key)
{
    PrefsNode *node;

    node = group->Next;
    while (node->Type > kNodeGroup) {
        if (node->Type == kNodeKey) {
            if (stricmp((char *)node + kKeyName, key) == 0)
                return node;
        }
        node = node->Next;
    }
    return 0;
}

/* The value is unquoted into the caller's buffer first; only if that came
   back unchanged AND the value is non-empty does the value replace the
   default, and the copy below runs on whichever survived. */
// FUNCTION: EVNOVA 0x004fff90
void PrefsGetString(PrefsNode *group, const char *key, const char *dflt,
                    char *out, int32_t outSize)
{
    PrefsNode *node;
    char *value;
    ULong len;

    node = PrefsFindEntry(group, key);
    value = node != 0 ? node->ValueText : 0;
    if (value != 0) {
        if (ParseQuotedString(out, outSize, value) != value)
            return;
        if (*value != 0)
            dflt = value;
    }
    if (outSize != 0) {
        if (dflt != 0) {
            len = strlen(dflt);
            if (len >= outSize)
                len = outSize - 1;
            memcpy(out, dflt, len);
        } else {
            len = 0;
        }
        out[len] = 0;
    }
}

/* PrefFileSectionAddEntry (BurgerLib PgPrefFile): "key = value" as the last line of the
   section group starts. */
// FUNCTION: EVNOVA 0x00500080
void PrefFileSectionAddEntry(PrefsNode *group, const char *key, const char *value)
{
    PrefsNode *node;
    PrefsNode *next;
    Word length;

    length = strlen(key);
    node = (PrefsNode *)AllocAPointer(length + kKeyLineSize);
    if (node != 0) {
        memcpy((char *)node + kKeyName, key, length + 1);
        node->ValueText = StrCopy(value);
        node->Type = kNodeKey;
        next = group->Next;
        while (next->Type > kNodeGroup)
            next = next->Next;
        node->Next = next;
        node->Prev = next->Prev;
        node->Next->Prev = node;
        node->Prev->Next = node;
    }
}

#ifdef __cplusplus
}
#endif
