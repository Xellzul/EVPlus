/* burgertick.cpp -- BurgerLib TkWin95Code: the 60 Hz tick counter. Notes: Research/notes/burgertick.md */

#include "burger.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint64_t TickStart;                       /* the clock at the first ReadTick */
static Word8 Started;                            /* 0x00865864 */

#define kTickFraction   8                        /* milliseconds kept in 24.8 fixed point */
#define kTickPeriod     0x10aa                   /* 1000/60 ms in 24.8: 16.664 */
#define kMicrosPerMs    1000
#define kWaitStepMs     1                        /* WaitTicks looks at the tick count this often */

/* ReadTick (BurgerLib TkWin95Code): ticks of 16.664 ms since the first call. The exe counts them on a
   timer thread (MyIrq, 0x004d5d40) that sleeps 16 ms, reads GetTickCount and adds the periods that have
   passed; here the same count is worked out from the clock when it is asked for. */
// FUNCTION: EVNOVA 0x004d5e10
Word32 ReadTick(void)
{
    if (Started == 0) {
        TickStart = PlatMicroseconds();
        Started = 1;
        return 0;
    }
    return (Word32)(((PlatMicroseconds() - TickStart) << kTickFraction) / (kTickPeriod * kMicrosPerMs));
}

/* WaitTicks (BurgerLib TkWin95Code). The exe sleeps in WaitMessage and has the timer thread post a message
   at each tick; here the wait ends at an event or after a millisecond, so that it returns as the tick
   count changes and not up to a tick later. */
// FUNCTION: EVNOVA 0x004d5e90
void WaitTicks(Word tickCount)
{
    Word32 now;

    KeyboardKbhit();
    now = ReadTick();
    while (now - LastTick < tickCount) {
        PlatWaitEvent(kWaitStepMs);
        KeyboardKbhit();
        now = ReadTick();
    }
    LastTick = now;
}

// FUNCTION: EVNOVA 0x004d6000
ULong ReadTickMilliseconds(void)
{
    return (ULong)(PlatMicroseconds() / kMicrosPerMs);
}

#ifdef __cplusplus
}
#endif
