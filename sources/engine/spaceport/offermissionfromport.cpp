/* offermissionfromport.cpp -- ResetMissionOfferVenue, OfferMissionFromPort; exe 0x00448660..0x00448670. Notes: Research/notes/gameutils.md */

#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"

// FUNCTION: EVNOVA 0x00448660
// MAC: _ResetMissionOfferVenue i386 0x00099f02 ppc 0x000a5d00
void ResetMissionOfferVenue(void)
{
    lastPortMissionLoc = -1;
}

// FUNCTION: EVNOVA 0x00448670
// MAC: _OfferMissionFromPort i386 0x000a369e ppc 0x000af148
Boolean OfferMissionFromPort(short stellar)
{
    short slot;
    short i;
    short mission;
    short first;

    if (stellar != lastPortMissionLoc) {
        if (stellar != 3) {
            for (i = 0; i < kMaxMissions; i++)
                suppressMissionOffer[i] = 0;
        }
        lastPortMissionLoc = stellar;
    }
    first = dockedPortMissions[1][0];
    if (first < 0 || first >= kMaxMissions)
        return 0;

    slot = -1;
    for (i = 0; i < kMaxMissions; i++) {
        mission = dockedPortMissions[1][i];
        if (mission != -1 && stellar == missionAvailData[mission].availLoc
                && suppressMissionOffer[mission] == 0
                && IsMissionAvailable(mission, 1)) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return 0;

    if (OfferOneMission(dockedPortMissions[1][slot]) == -1) {
        suppressMissionOffer[dockedPortMissions[1][slot]] = 1;
    } else {
        for (i = slot; i < kMaxMissions - 1; i++)
            dockedPortMissions[1][i] = dockedPortMissions[1][i + 1];
    }
    missionOfferTime = ReadTick() + Rand(30) + 30;
    return 1;
}
