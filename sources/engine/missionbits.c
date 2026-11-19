// COMPILE: -lang c++ -bool off
/* missionbits.c -- mission-bit set-expression entry points (win TU region around 0x00448020). Notes: Research/notes/missionbits.md */
#include <stdio.h>
#include "burger.h"
#include "missions.h"
#include "player.h"
#include "game.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00448020
// MAC: _EvalMissionBitSetString i386 0x00099dc5 ppc 0x000a5b94
void EvalMissionBitSetString(char *expr)
{
    if (strlen(expr) == 0)
        return;
    strcpy(parseBuffer, expr);
    EvalSetExp();
    ResetPlayerPrecalcedValues();
}

// FUNCTION: EVNOVA 0x00448050
// MAC: _EvalCurrentMissionBitSetString i386 0x00099a43 ppc 0x000a5748
void EvalCurrentMissionBitSetString(char *expr, short mission)
{
    if (strlen(expr) == 0)
        return;
    strcpy(parseBuffer, expr);
    currentMissionIndexForSetParser = mission;
    EvalSetExp();
    currentMissionIndexForSetParser = -1;
    ResetPlayerPrecalcedValues();
}

EXTERN_C_END
