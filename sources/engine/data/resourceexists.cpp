/* resourceexists.cpp -- ResourceExists; exe 0x0047b7a0. Notes: Research/notes/resmgr.md */

#include "gameutil.h"

/* engine-facing wrapper; lives at 0x0047b7a0, grouped here by topic */
// FUNCTION: EVNOVA 0x0047b7a0
Boolean ResourceExists(uint32_t type, short id)
{
    return nv_ResourceExists(type, id);
}
