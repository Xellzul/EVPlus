/* atticks.cpp -- the platform layer's clocks: the millisecond tick count and its rate.
   The exe reads the performance counter, corrects it against GetTickCount when the two drift more than
   half a second apart, and keeps a timer thread that touches the counter every 4 s; here it reads the
   platform's one monotonic clock. Notes: Research/notes/atticks.md */

#include "burger.h"
#include "platform.h"

#define kMicrosPerMilli 1000

/* Milliseconds (since boot in the exe); only differences are taken. */
// FUNCTION: EVNOVA 0x004d3ae6
ULong TimerGetTicks(void)
{
    return (ULong)(PlatMicroseconds() / kMicrosPerMilli);
}

/* The tick rate that goes with it. */
// FUNCTION: EVNOVA 0x004d3af5
ULong TimerGetTickRate(void)
{
    return kMillisPerSecond;
}
