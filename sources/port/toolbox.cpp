/* toolbox.cpp -- small Mac Toolbox calls the game makes (they came with QuickTime for Windows): the clocks,
   QuickDraw's random numbers, StringToNum, BlockMove, DebugStr and the main device. */

#include "burger.h"
#include "platform.h"
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
