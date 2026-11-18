// COMPILE: -lang c++ -bool off
/* burgertick.c -- BurgerLib TkWin95Code: the 60 Hz tick counter kept by a timer thread. Notes: Research/notes/burgertick.md */

#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

static volatile Word32 TimeCount;                /* 0x00865858 */
static Word32 TimerID;                           /* 0x0086585c */
static void *ThreadHandle;                       /* 0x00865860 */
static Word8 Started;                            /* 0x00865864 */
static Word GotMilk;                             /* 0x00865868 */

#define kTickFraction   8                        /* milliseconds kept in 24.8 fixed point */
#define kTickPeriod     0x10aa                   /* 1000/60 ms in 24.8: 16.664 */
#define kTickSleepMs    16
#define kTickStackSize  0x400
#define kTimerPriority  2                        /* THREAD_PRIORITY_HIGHEST */

/* MyIrq (BurgerLib TkWin95Code, the timer thread). */
// FUNCTION: EVNOVA 0x004d5d40
static Word32 __stdcall MyIrq(void *data)
{
    Word32 mark;
    Word32 elapsed;

    mark = GetTickCount() << kTickFraction;
    for (;;) {
        do {
            SleepEx(kTickSleepMs, 0);
            elapsed = (GetTickCount() << kTickFraction) - mark;
        } while (elapsed < kTickPeriod);
        do {
            ++TimeCount;
            mark += kTickPeriod;
            elapsed -= kTickPeriod;
        } while (elapsed >= kTickPeriod);
    }
}

/* MyShutdown (BurgerLib TkWin95Code, the atexit hook). */
// FUNCTION: EVNOVA 0x004d5df0
static void MyShutdown(void)
{
    CloseHandle(ThreadHandle);
    ThreadHandle = 0;
}

/* ReadTick (BurgerLib TkWin95Code). */
// FUNCTION: EVNOVA 0x004d5e10
Word32 ReadTick(void)
{
    if (Started != 0)
        return TimeCount;
    atexit(MyShutdown);
    ThreadHandle = CreateThread(0, kTickStackSize, MyIrq, 0, 0, &TimerID);
    SetThreadPriority(ThreadHandle, kTimerPriority);
    Started = 1;
    return TimeCount;
}

/* WaitTicks (BurgerLib TkWin95Code). The exe sleeps in WaitMessage and has the timer thread post a message
   at each tick; here the wait ends at an event or after one tick's time. */
// FUNCTION: EVNOVA 0x004d5e90
void WaitTicks(Word tickCount)
{
    Word32 now;

    KeyboardKbhit();
    now = ReadTick();
    while (now - LastTick < tickCount) {
        PlatWaitEvent(kTickSleepMs);
        KeyboardKbhit();
        now = ReadTick();
    }
    LastTick = now;
}

#define kMicrosPerSecond 1000000.0

static double WinTicks = 1.0;                    /* microseconds per counter tick */

/* ReadTickMicroseconds (BurgerLib TkWin95Code): not linked, dead-stripped in the exe; it stays
   for its 1000000.0 at 0x005766e8, the TU's one kept .data (notes) */
Word32 ReadTickMicroseconds(void)
{
    long long count;

    if (!GotMilk) {
        GotMilk = 1;
        if (QueryPerformanceFrequency(&count))
            WinTicks = kMicrosPerSecond / count;
    }
    if (QueryPerformanceCounter(&count))
        return count * WinTicks;
    return 0;
}

// FUNCTION: EVNOVA 0x004d6000
ULong ReadTickMilliseconds(void)
{
    return timeGetTime();
}

#ifdef __cplusplus
}
#endif
