/* text.cpp -- std::string text helpers (text.h) */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
#include "gameutil.h"
#include "missions.h"
#include "dialogs.h"
#include "prototypes.h"

#define kDateStrings    137     /* STR# holding the month names and day suffixes */
#define kFirstMonthStr   12     /* the short name of month 1 is entry 13 */
#define kSuffixSt        25
#define kSuffixNd        26
#define kSuffixRd        27
#define kSuffixTh        28

std::string FromPascal(const unsigned char *p)
{
    if (p == NULL)
        return std::string();
    return std::string((const char *)p + 1, p[0]);
}

std::string FromPascal(const void *p, size_t fieldSize)
{
    const unsigned char *s = (const unsigned char *)p;
    size_t len;

    if (fieldSize == 0)
        return std::string();
    len = s[0];
    if (len > fieldSize - 1)
        len = fieldSize - 1;
    return std::string((const char *)s + 1, len);
}

std::string FromNameField(const void *p, size_t fieldSize)
{
    std::string text = FromPascal(p, fieldSize);
    size_t nul = text.find('\0');

    if (nul != std::string::npos)
        text.resize(nul);
    return text;
}

std::string FromField(const char *field, size_t fieldSize)
{
    return std::string(field, strnlen(field, fieldSize));
}

void ToPascal(const std::string &text, unsigned char (&out)[256])
{
    size_t len = text.size() < 255 ? text.size() : 255;

    out[0] = (unsigned char)len;
    memcpy(out + 1, text.data(), len);
    if (len < 255)
        out[len + 1] = 0;
}

std::string UpToNul(const std::string &text, size_t maxLen)
{
    size_t len = text.size() < maxLen ? text.size() : maxLen;
    size_t nul = text.find('\0');

    if (nul < len)
        len = nul;
    return text.substr(0, len);
}

std::string TextF(const char *fmt, ...)
{
    std::string text;
    va_list args;
    va_list count;
    int len;

    va_start(args, fmt);
    va_copy(count, args);
    len = vsnprintf(NULL, 0, fmt, count);
    va_end(count);
    if (len > 0) {
        text.resize((size_t)len);
        vsnprintf(&text[0], (size_t)len + 1, fmt, args);
    }
    va_end(args);
    return text;
}

void CopyCField(void *dst, size_t dstSize, const char *src, size_t srcSize)
{
    size_t len;

    if (dstSize == 0)
        return;
    len = strnlen(src, srcSize);
    if (len > dstSize - 1)
        len = dstSize - 1;
    memcpy(dst, src, len);
    ((char *)dst)[len] = 0;
}

void CopyPascalName(void *dst, const unsigned char *src, size_t count)
{
    unsigned char *p = (unsigned char *)dst;

    if (count == 0)
        return;
    strncpy((char *)p, (const char *)src, count);
    if (p[0] > count - 1)
        p[0] = (unsigned char)(count - 1);
}

void CopyPascalNameSigned(char *dst, const unsigned char *src, size_t count)
{
    if (count == 0)
        return;
    strncpy(dst, (const char *)src, count);
    if (dst[0] > 0 && (size_t)dst[0] > count - 1)
        dst[0] = (char)(count - 1);
}

void PascalToCBuffer(char *dst, size_t dstSize, const unsigned char *src)
{
    size_t len = src[0];

    if (dstSize == 0)
        return;
    if (len > dstSize - 1)
        len = dstSize - 1;
    len = strnlen((const char *)src + 1, len);
    memcpy(dst, src + 1, len);
    dst[len] = 0;
}

std::string IndString(int list, int index)
{
    Str255 str;
    std::string text;
    size_t nul;

    GetIndString(str, list, index);
    text = FromPascal(str);
    nul = text.find('\0');
    if (nul != std::string::npos)
        text.resize(nul);
    return text;
}

std::string NumText(int32_t value)
{
    Str255 str;

    NumToString(value, str);
    return FromPascal(str);
}

std::string CommaNumber(int32_t value)
{
    if (value < 1000)
        return NumText(value);
    if (value < 1000000)
        return TextF("%d,%03d", (int)(value / 1000), (int)(value % 1000));
    return TextF("%d.%02dM", (int)(value / 1000000), (int)(value % 1000000 / 10000));
}

std::string OrdinalText(short n, bool capitalize)
{
    std::string word = (n >= 1 && n <= 10) ? IndString(kOrdinalsSTR, n + kOrdinalsBase) : NumText(n);

    if (capitalize && !word.empty())
        word[0] = (char)toupper((unsigned char)word[0]);
    return word;
}

/* "<prefix><month> <day><suffix>, <year><suffix>" with the month from STR# 137 entry `month' */
static std::string DateText(const DateTimeRec &date, int month)
{
    short suffix = kSuffixTh;

    if (date.day % 10 == 1)
        suffix = kSuffixSt;
    if (date.day % 10 == 2)
        suffix = kSuffixNd;
    if (date.day % 10 == 3)
        suffix = kSuffixRd;
    if (date.day >= 11 && date.day <= 13)
        suffix = kSuffixTh;
    return TextF("%s%s %d%s, %d%s", FromField(datePrefix, sizeof datePrefix).c_str(),
                 IndString(kDateStrings, month).c_str(), date.day, IndString(kDateStrings, suffix).c_str(), date.year,
                 FromField(dateSuffix, sizeof dateSuffix).c_str());
}

std::string DateLongText(const DateTimeRec &date)
{
    return DateText(date, date.month);
}

std::string DateMedText(const DateTimeRec &date)
{
    return DateText(date, date.month + kFirstMonthStr);
}

void SetShowText(const std::string &text)
{
    CopyCField(textShowString, sizeof textShowString, text.c_str(), text.size());
}

/* -- record names -- */

std::string SpobName(int i)
{
    if (stellarData == NULL || i < 0 || i >= kMaxStellars)
        return std::string();
    return FromNameField(stellarData[i].name);
}

std::string SystName(int i)
{
    if (systemData == NULL || i < 0 || i >= kMaxSystems)
        return std::string();
    return FromNameField(systemData[i].name);
}

std::string GovtName(int i)
{
    if (govtData == NULL || i < 0 || i >= kMaxGovts)
        return std::string();
    return FromNameField(govtData[i].name);
}

std::string OutfitName(int i)
{
    if (outfitSystem == NULL || i < 0 || i >= kMaxOutfits)
        return std::string();
    return FromNameField(outfitSystem[i].name);
}

std::string ShipClassName(int i)
{
    if (shipClass == NULL || i < 0 || i >= kMaxShipTypes)
        return std::string();
    return FromNameField(shipClass[i].name);
}

std::string PersonName(int i)
{
    if (personData == NULL || i < 0 || i >= kMaxPersons)
        return std::string();
    return FromNameField(personData[i].name);
}

std::string RankName(int i)
{
    if (rankData == NULL || i < 0 || i >= kMaxRanks)
        return std::string();
    return FromField(rankData[i].name, sizeof rankData[i].name);
}

std::string RankConvName(int i)
{
    if (rankData == NULL || i < 0 || i >= kMaxRanks)
        return std::string();
    return FromField(rankData[i].convName, sizeof rankData[i].convName);
}

std::string RankShortName(int i)
{
    if (rankData == NULL || i < 0 || i >= kMaxRanks)
        return std::string();
    return FromField(rankData[i].shortName, sizeof rankData[i].shortName);
}

std::string JunkName(int i)
{
    if (junkData == NULL || i < 0 || i >= kMaxJunks)
        return std::string();
    return FromNameField(junkData[i].name);
}

std::string JunkLcName(int i)
{
    if (junkData == NULL || i < 0 || i >= kMaxJunks)
        return std::string();
    return FromField(junkData[i].lcName, sizeof junkData[i].lcName);
}

std::string DisasterName(int i)
{
    if (disasterData == NULL || i < 0 || i >= kMaxDisasters)
        return std::string();
    return FromNameField(disasterData[i].name);
}

static const MissionRecord *MissionSlot(int slot)
{
    if (currentMissions == NULL || slot < 0 || slot >= kMaxActiveMissions)
        return NULL;
    return &currentMissions[slot];
}

std::string MissionName(int slot)
{
    const MissionRecord *m = MissionSlot(slot);

    return m != NULL ? FromNameField(m->name) : std::string();
}

std::string MissionShipName(int slot)
{
    const MissionRecord *m = MissionSlot(slot);

    return m != NULL ? FromNameField(m->shipName) : std::string();
}

std::string MissionShipSubtitle(int slot)
{
    const MissionRecord *m = MissionSlot(slot);

    return m != NULL ? FromNameField(m->shipSubtitle) : std::string();
}

bool MissionHasShipName(int slot)
{
    const MissionRecord *m = MissionSlot(slot);

    return m != NULL && (signed char)m->shipName[0] > 0;
}

bool MissionHasShipSubtitle(int slot)
{
    const MissionRecord *m = MissionSlot(slot);

    return m != NULL && (signed char)m->shipSubtitle[0] > 0;
}
