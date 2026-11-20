/* nvresourceexists.cpp -- nv_ResourceExists; exe 0x004ce640. Notes: Research/notes/resmgr.md */

#include "gameutil.h"

/* nv_ResourceExists (mac (port layer)). */
// FUNCTION: EVNOVA 0x004ce640
Boolean nv_ResourceExists(uint32_t type, short id)
{
    return nv_FindResourceRecord(type, id) != NULL;
}
