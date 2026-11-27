/* text.h -- std::string text (engine/utils/text.cpp): conversion from and to the game's Pascal and C buffers,
   STR# entries, numbers and dates, record names, and std::string forms of the text calls. A string holds the
   game's own bytes: Mac Roman from the data, typed text as it was typed. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include "macport.h"

namespace UI {
struct TextSpec;
}

/* -- conversion: text enters and leaves std::string only through these -- */

/* the p[0] bytes after the length byte; NULL gives "" */
std::string FromPascal(const unsigned char *p);
/* a Pascal string in a field of fieldSize bytes: at most fieldSize - 1 bytes, whatever its length byte says */
std::string FromPascal(const void *p, size_t fieldSize);
/* a record's name field (Pascal, fieldSize bytes) as its readers see it: bounded by the field, up to a NUL */
std::string FromNameField(const void *p, size_t fieldSize);
template <typename C, size_t N>
std::string FromNameField(const C (&field)[N])
{
    static_assert(sizeof(C) == 1, "FromNameField reads a byte field");
    return FromNameField(field, N);
}
/* a C string in a field of fieldSize bytes: up to its NUL, at most fieldSize bytes */
std::string FromField(const char *field, size_t fieldSize);
/* text as a Pascal string, cut to 255 bytes */
void ToPascal(const std::string &text, unsigned char (&out)[256]);
/* the bytes before text's first NUL, at most maxLen */
std::string UpToNul(const std::string &text, size_t maxLen = std::string::npos);
/* printf into a std::string; game or plug-in text goes in only as a %s argument, never as fmt */
std::string TextF(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* a string literal as a Pascal string (length byte, the bytes, a NUL) for the calls that take one:
   PascalLiteral("abc").text, valid to the end of the full expression */
template <size_t N>
struct PascalLiteral {
    unsigned char text[N + 1];

    constexpr PascalLiteral(const char (&s)[N]) : text()
    {
        static_assert(N >= 1 && N <= 256, "a Pascal string holds 255 bytes");
        text[0] = (unsigned char)(N - 1);
        for (size_t i = 0; i < N; i++)
            text[i + 1] = (unsigned char)s[i];
    }
};

/* -- bounded copies into the game's fixed buffers -- */

/* a C string field of srcSize bytes (its NUL may be missing) into dst: up to the NUL or the field's end, cut to
   dstSize - 1 bytes, then a NUL */
void CopyCField(void *dst, size_t dstSize, const char *src, size_t srcSize);
template <typename D, size_t N, size_t M>
void CopyCField(D (&dst)[N], const char (&src)[M])
{
    static_assert(sizeof(D) == 1, "CopyCField writes a byte field");
    CopyCField(dst, N, src, M);
}
/* strncpy(dst, src, count) of a Pascal name, its length byte cut to the count - 1 bytes after it */
void CopyPascalName(void *dst, const unsigned char *src, size_t count);
/* the same for a char field whose readers test the length byte as signed: 128..255 (no name to them) stays */
void CopyPascalNameSigned(char *dst, const unsigned char *src, size_t count);
/* a Pascal string as a C string: its bytes up to a NUL, cut to dstSize - 1 */
void PascalToCBuffer(char *dst, size_t dstSize, const unsigned char *src);

/* -- resources, numbers, dates -- */

/* entry `index' of STR# `list' up to an embedded NUL; empty when the list or entry is missing */
std::string IndString(int list, int index);
/* the whole entry, a NUL in it and the bytes after it too: what the entry is drawn as */
std::string IndStringRaw(int list, int index);
/* NumToString's digits, '-' first when negative */
std::string NumText(int32_t value);
/* 12,345 below a million, 1.23M (two decimals, cut) from a million; a negative value is plain digits */
std::string CommaNumber(int32_t value);
/* "first".."tenth" (STR# 137) for 1..10, digits otherwise; capitalize = upper-case first letter */
std::string OrdinalText(short n, bool capitalize);
/* "<prefix>May 1st, 1177<suffix>" with the pilot's date prefix and suffix, the month in full / short */
std::string DateLongText(const DateTimeRec &date);
std::string DateMedText(const DateTimeRec &date);

/* table[i] of a string table, or an empty string for an index out of range */
template <size_t N>
const std::string &TableEntry(const std::string (&table)[N], int i)
{
    static const std::string empty;

    return i >= 0 && (size_t)i < N ? table[i] : empty;
}

/* -- record names: empty for an index out of range; Pascal fields bounded by their size and cut at a NUL -- */

std::string SpobName(int i);
std::string SystName(int i);
std::string GovtName(int i);
std::string OutfitName(int i);
std::string ShipClassName(int i);
std::string PersonName(int i);
std::string RankName(int i);
std::string JunkName(int i);
std::string JunkLcName(int i);
/* the active mission in slot `slot' */
std::string MissionShipName(int slot);
std::string MissionShipSubtitle(int slot);
/* the callers' test for a mission ship name / subtitle: its length byte read as signed, above 0 */
bool MissionHasShipName(int slot);
bool MissionHasShipSubtitle(int slot);

/* -- the text calls on std::string -- */

/* textShowString (ShowTextDialog's text) set to the text up to a NUL, cut to the buffer */
void SetShowText(const std::string &text);
/* cut to 255 bytes; a char buffer must be passed as std::string or cast to its Pascal type */
void DisplayComm(const std::string &msg, short count);
void DisplayCommStyled(const std::string &msg, short count, const UI::TextSpec &spec);
void DisplayComm(const char *msg, short count) = delete;
void DisplayCommStyled(const char *msg, short count, const UI::TextSpec &spec) = delete;
