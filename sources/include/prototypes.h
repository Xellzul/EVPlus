/* prototypes.h -- the four names that cannot yet have a subsystem home. Notes: Research/notes/prototypes.md */
#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include "shipobj.h"

extern void StrcatIndCString(char *dst, short strListID, short index);   /* 0x004cd1a0 mac _StrcatIndCString */
extern MissionRecord *currentMissions;               /* 0x005914b4 (mac _currentMissions), initevnova.cpp */
extern BoxObj *boxData;                              /* 0x005914a8 */

#endif /* PROTOTYPES_H */
