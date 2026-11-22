/* toolbox.cpp -- small Mac Toolbox calls the game makes: the clocks, QuickDraw's Random, StringToNum and NumToString,
   BlockMove, DebugStr, the main device, GetIndString, ParamText and Alert, Delay, ReplaceInHandle, ExitToShell */

#include <time.h>

#include "burger.h"
#include "cefix.h"
#include "endianswap.h"
#include "platform.h"
#include "player.h"
#include "spriteworld.h"

#define kDeviceToken      0x51544744                /* any non-NULL "GDHandle"; nothing reads it */

int32_t GetMainDevice(void) { return kDeviceToken; }

void Microseconds(UnsignedWide *now)
{
    uint64_t us = PlatMicroseconds();

    now->lo = (uint32_t)us;
    now->hi = (uint32_t)(us >> 32);
}

void GetDateTime(uint32_t *secs)
{
    *secs = PlatLocalSeconds1904();
}

/* measured: optional sign, then decimal digits up to the first other character (" 12" is 0, "12a3" is 12),
   wrapping on overflow */
void StringToNum(const unsigned char *str, int32_t *value)
{
    uint32_t v = 0;
    int i = 1, n = str[0], neg = 0;

    if (i <= n && (str[i] == '-' || str[i] == '+')) {
        neg = str[i] == '-';
        i++;
    }
    for (; i <= n && str[i] >= '0' && str[i] <= '9'; i++)
        v = v * 10u + (uint32_t)(str[i] - '0');
    *value = (int32_t)(neg ? 0u - v : v);
}

/* QuickDraw's generator; randSeed starts at 1 (measured: 16807, 15089, -21287, ...) */
static uint32_t gRandSeed = 1;

short Random(void)
{
    short r;

    gRandSeed = (uint32_t)(((uint64_t)gRandSeed * 16807u) % 0x7fffffffu);
    r = (short)(gRandSeed & 0xffff);
    if (r == -32768)
        r = 0;
    return r;
}

int32_t GetQDGlobalsRandomSeed(void)
{
    return (int32_t)gRandSeed;
}

void SetQDGlobalsRandomSeed(int32_t seed)
{
    gRandSeed = (uint32_t)seed;
}

void BlockMove(const void *src, void *dst, uint32_t len)
{
    memmove(dst, src, len);
}

void DebugStr(const char *message)
{
    char buf[260];
    int n = (unsigned char)message[0];

    memcpy(buf, message + 1, (size_t)n);
    buf[n] = '\0';
    PlatDebugLog(buf);
}

/* The STR# resource's string. `entry' is seeded from `base' before the count is read (`entry = base + 2'
   coalesces the pair). */

// FUNCTION: EVNOVA 0x004b8ca0
void GetIndString(unsigned char *str, int strListID, int index)
{
    Handle h;
    unsigned char *entry;
    unsigned short count;
    unsigned char *base;

    str[0] = 0;
    h = GetResource(kStringListType, strListID);
    if (h == NULL)
        return;
    LockAHandle(h);
    base = (unsigned char *)*h;
    entry = base;
    count = SwapEndianShort(&((StringListRec *)base)->count);
    entry += 2;
    if (index != 0 && index <= count) {
        while (--index != 0)
            entry += *entry + 1;
        memcpy(str, entry, *entry + 1);
    }
    ReleaseResource(h);
}

/* GetTime (mac (Toolbox GetTime shim)). */
// FUNCTION: EVNOVA 0x004bbb10
void GetTime(DateTimeRec *dtr)
{
    time_t now;
    struct tm *t;

    now = time(NULL);
    t = localtime(&now);
    dtr->year = t->tm_year + 1900;
    dtr->month = t->tm_mon + 1;
    dtr->day = t->tm_mday;
    dtr->hour = t->tm_hour;
    dtr->minute = t->tm_min;
    dtr->second = t->tm_sec;
    dtr->dayOfWeek = t->tm_wday + 1;
}

// FUNCTION: EVNOVA 0x004bbce0
void NumToString(int32_t value, unsigned char *str)
{
    char text[256];

    CE_NumberToAscii(text, value, 0);
    CStr2PStr((char *)str, text);
}

/* the window and GL go before exit(): libraries' own exit handlers run before ours */
// FUNCTION: EVNOVA 0x004bbd30
void ExitToShell(void)
{
    ShutdownWinGame();
    PlatQuit();
    exit(0);
}

char gParamText0[256];
char gParamText1[256];
char gParamText2[256];
char gParamText3[256];

#define kResourceErrorCode 0x83

// FUNCTION: EVNOVA 0x004bbf90
void ParamText(const unsigned char *p0, const unsigned char *p1,
               const unsigned char *p2, const unsigned char *p3)
{
    gParamText0[0] = 0;
    gParamText1[0] = 0;
    gParamText2[0] = 0;
    gParamText3[0] = 0;
    if (p0 != NULL && p0[0] != 0) {
        memcpy(gParamText0, p0 + 1, p0[0]);
        gParamText0[p0[0]] = 0;
    }
    if (p1 != NULL && p1[0] != 0) {
        memcpy(gParamText1, p1 + 1, p1[0]);
        gParamText1[p1[0]] = 0;
    }
    if (p2 != NULL && p2[0] != 0) {
        memcpy(gParamText2, p2 + 1, p2[0]);
        gParamText2[p2[0]] = 0;
    }
    if (p3 != NULL && p3[0] != 0) {
        memcpy(gParamText3, p3 + 1, p3[0]);
        gParamText3[p3[0]] = 0;
    }
}

// FUNCTION: EVNOVA 0x004bc070
short Alert(short code, void *filterProc)
{
    char message[1024];

    if (code == kResourceErrorCode)
        sprintf(message, "Resource ID:  %s\nCode: %s\n%s",
                gParamText0, gParamText1, gParamText2);
    else
        sprintf(message, "%s %s %s", gParamText0, gParamText1, gParamText2);
    ShowAlert("", message);
    return 1;
}

// FUNCTION: EVNOVA 0x004bc0e0
void Delay(uint32_t ticks, uint32_t *finalTicks)
{
    LastTick = ReadTick();
    WaitTicks(ticks);
}

// FUNCTION: EVNOVA 0x004bc100
int32_t ReplaceInHandle(char **h, int32_t offset, const char *find, int32_t findLen,
                      const char *repl, int32_t replLen)
{
    char *start;
    int32_t count;
    char c;
    char *p;
    int32_t oldSize;
    int32_t newSize;
    int32_t n;
    int handleSize;

    LockAHandle(h);
    start = *h + offset;
    count = GetAHandleSize(h) - offset - findLen + 1;
    if (count <= 0) {
        return -1;
    }
    c = find[0];
    p = (char *)memchr(start, c, count);
    while (p != NULL) {
        if (memcmp(p, find, findLen) == 0) {
            oldSize = GetAHandleSize(h);
            handleSize = oldSize;
            newSize = oldSize + replLen - findLen;
            offset += p - start;
            if (newSize > oldSize) {
                ResizeAHandle(h, newSize);
                LockAHandle(h);
                p = *h + offset;
            }
            if (replLen != findLen) {
                n = handleSize - offset - findLen;
                if (n != 0)
                    memmove(p + replLen, p + findLen, n);
            }
            memcpy(p, repl, replLen);
            if (newSize < oldSize)
                ResizeAHandle(h, newSize);
            return offset + replLen;
        }
        p++;
        count -= p - start;
        offset += p - start;
        start = p;
        p = (char *)memchr(start, c, count);
    }
    return -1;
}

// FUNCTION: EVNOVA 0x004bc300
void SysBeep(short duration)
{
}
