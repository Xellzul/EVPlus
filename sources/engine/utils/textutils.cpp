/* textutils.cpp -- number, date and name formatting, Pascal strings, status strings */

#include "player.h"
#include "render.h"
#include "gameutil.h"
#include "prototypes.h"
#include "missions.h"
#include "text.h"

// FUNCTION: EVNOVA 0x00465af0
// MAC: _DrawCommaNumber i386 0x00004895 ppc 0x00005cb0
void DrawCommaNumber(int32_t value)
{
    int32_t group;
    unsigned char str[256];

    if (value < 1000) {
        NumToString(value, str);
        DrawString(str);
    } else if (value < 1000000) {
        NumToString(value / 1000, str);
        DrawString(str);
        DrawCString(",");
        if (value % 1000 < 100)
            DrawCString("0");
        if (value % 1000 < 10)
            DrawCString("0");
        NumToString(value % 1000, str);
        DrawString(str);
    } else {
        NumToString(value / 1000000, str);
        DrawString(str);
        DrawCString(".");
        group = value % 1000000;
        NumToString(group / 10000, str);
        if (group / 10000 < 10)
            DrawCString("0");
        DrawString(str);
        DrawCString("M");
    }
}

// FUNCTION: EVNOVA 0x00465c10
// MAC: _StrcatCommaNumber i386 0x00008820 ppc 0x0000a8e0
void StrcatCommaNumber(int32_t value, char *dst)
{
    strcat(dst, CommaNumber(value).c_str());
}

/* Append an ordinal to a C string: first through tenth out of STR# 137 (its entries 29..38), anything else in
   digits. */
// FUNCTION: EVNOVA 0x00465d90
// MAC: _StrcatOrdinalNumber i386 0x00008ab4 ppc 0x0000ab98
void StrcatOrdinalNumber(short n, char *dst, Boolean capitalize)
{
    strcat(dst, OrdinalText(n, capitalize != 0).c_str());
}


/* MSL <ctype.h> inlines tolower's table lookup; the exe calls the library
   function (0x004d6230), so declare it without the header. */

// FUNCTION: EVNOVA 0x004665f0
// MAC: _CullNameString i386 0x0000dbe2 ppc 0x00011150
void CullNameString(unsigned char *name)
{
    short skip;
    short len;
    short i;
    short count;
    unsigned char work[256];
    unsigned char culled[256];

    pstrcpy(work, name);
    PtoCstr(work);
    skip = 0;
    len = strlen((char *)work);
    count = len;
    if (tolower(work[0]) == 't' && tolower(work[1]) == 'h'
            && tolower(work[2]) == 'e') {
        if (work[3] == ' ') {
            skip += 4;
            count -= 4;
        }
    }
    for (i = 0; i < count; i++)
        culled[i] = work[i + skip];
    culled[i] = 0;
    CtoPstr((char *)culled);
    pstrcpy(name, culled);
}

// FUNCTION: EVNOVA 0x00468450
// MAC: _DateToMedString i386 0x00008cb6 ppc 0x0000adcc
void DateToMedString(DateTimeRec date, char *out)
{
    strcpy(out, DateMedText(date).c_str());
}

// FUNCTION: EVNOVA 0x00468600
// MAC: _DateToLongString i386 0x00008ef8 ppc 0x0000b014
void DateToLongString(DateTimeRec date, char *out)
{
    strcpy(out, DateLongText(date).c_str());
}

// FUNCTION: EVNOVA 0x00468d90
// MAC: _DrawLegalStatusString i386 0x0000d43f ppc 0x00010888
void DrawLegalStatusString(short sys)
{
    int32_t base;
    short haven;
    short clean;
    int32_t status;
    short j;
    short rating;

    status = playerRecord[sys];
    rating = 0;
    if (systemData[sys].govt >= 0 && systemData[sys].govt < kMaxGovts)
        base = govtData[systemData[sys].govt].crimeTol;
    else
        base = govtData[0].crimeTol;

    if (status < 0)
        rating = 2;
    if (status < -base)
        rating = 3;
    if (status < -base * 4)
        rating = 4;
    if (status < -base * 16)
        rating = 5;
    if (status < -base * 64)
        rating = 6;
    if (status < -base * 256)
        rating = 7;
    if (status < -base * 1024)
        rating = 8;
    if (status < -base * 4096)
        rating = 9;
    if (status == 0)
        rating = 1;
    if (status > 0)
        rating = 0xa;
    if (status > base * 4)
        rating = 0xb;
    if (status > base * 16)
        rating = 0xc;
    if (status > base * 64)
        rating = 0xd;
    if (status > base * 256)
        rating = 0xe;
    if (status > base * 1024)
        rating = 0xf;

    haven = clean = 0;
    for (j = 0; j < 3; j++) {
        if (systemData[sys].navDefault[j] != -1 && stellarData[systemData[sys].navDefault[j]].loaded &&
                !SpobUninhabited(&stellarData[systemData[sys].navDefault[j]]) &&
                StellarIsNormalLandable(&stellarData[systemData[sys].navDefault[j]])) {
            if (stellarData[systemData[sys].navDefault[j]].dominated != 0)
                haven++;
            else
                clean++;
        }
    }
    if (haven > 0) {
        if (clean > 0)
            rating = 0x10;
        else
            rating = 0x11;
    }

    if (systemData[sys].govt >= 0 && (govtData[systemData[sys].govt].flags & kGovtXenophobic) != 0)
        rating = 0;

    if (rating == 0)
        DrawIndString(kGameStrList, 0x18c);
    else
        DrawCString(legalStatusStrings[rating]);
}

// FUNCTION: EVNOVA 0x00469030
// MAC: _DrawCombatRatingString i386 0x000057e5 ppc 0x00006e68
void DrawCombatRatingString(void)
{
    short rating;

    rating = 0;
    if (playerKills > 0)
        rating = 1;
    if (playerKills >= 100)
        rating = 2;
    if (playerKills >= 200)
        rating = 3;
    if (playerKills >= 400)
        rating = 4;
    if (playerKills >= 800)
        rating = 5;
    if (playerKills >= 1600)
        rating = 6;
    if (playerKills >= 3200)
        rating = 7;
    if (playerKills >= 6400)
        rating = 8;
    if (playerKills >= 12800)
        rating = 9;
    if (playerKills >= 25600)
        rating = 10;
    DrawCString(ratingStrings[rating]);
}

// FUNCTION: EVNOVA 0x0046d250
// MAC: _PtoCstr i386 0x000049e2 ppc 0x00005e04
char *PtoCstr(unsigned char *str)
{
    short len = *str;
    short i;

    for (i = 0; i < len; i++)
        str[i] = str[i + 1];
    str[len] = 0;
    return (char *)str;
}

// FUNCTION: EVNOVA 0x0046d380
// MAC: _CtoPstr i386 0x00004d13 ppc 0x00006210
unsigned char *CtoPstr(char *str)
{
    short len = strlen(str);
    short i;

    for (i = len; i >= 1; i--)
        str[i] = str[i - 1];
    *str = len;
    return (unsigned char *)str;
}

/* src split at its \n marks (a backslash, then n or N): the text before the first into first, the rest into
   second; true when there was one */
// FUNCTION: EVNOVA 0x0046e6d0
// MAC: _SplitTwoLineString i386 0x00007dc1 ppc 0x00009b98
bool SplitTwoLineString(const std::string &src, std::string &first, std::string &second)
{
    bool afterBreak = false;
    size_t i;

    first.clear();
    second.clear();
    for (i = 0; i < src.size(); i++) {
        if (i + 1 < src.size() && src[i] == '\\' && (src[i + 1] == 'n' || src[i + 1] == 'N')) {
            i++;
            afterBreak = true;
        } else if (afterBreak) {
            second += src[i];
        } else {
            first += src[i];
        }
    }
    return afterBreak;
}

// FUNCTION: EVNOVA 0x0046eeb0
// MAC: _pstrcpy i386 0x00004ce2 ppc 0x000061d8
void pstrcpy(unsigned char *dst, const unsigned char *src)
{
    short i;

    dst[0] = src[0];
    for (i = 1; i <= src[0]; i++)
        dst[i] = src[i];
    if (i < (short)sizeof(Str255))
        dst[i] = 0;
}

/*
 * The comparison next to it at 0x0046ef00, and it is odd on purpose: the loop
 * bound is a logical AND of the two length bytes, so it is 1 for any non-empty
 * pair and the body only ever re-checks index 0. The length equality test in
 * front of it already decided the answer.
 */
// FUNCTION: EVNOVA 0x0046ef00
// MAC: _pstrcmp i386 0x000084d2 ppc 0x0000a554
short pstrcmp(const unsigned char *a, const unsigned char *b)
{
    short i;

    if (a[0] != b[0])
        return -1;
    for (i = 0; i < (a[0] && b[0]); i++) {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}
