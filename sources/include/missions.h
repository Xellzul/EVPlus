/*
 * missions.h -- missions, crons, mission bits and the comm traffic they drive.
 *
 * One declaration per name for the whole tree.
 * Addresses are the win 1.0.10 exe's.
 */
#ifndef MISSIONS_H
#define MISSIONS_H
#include "macport.h"
#include "resources.h"
#include <string.h>
#include <string>

/* mission system codes */
#define kSystemRandom       -2
#define kSystemAdjacent     -5      /* a random system one hyperlink from the current one */

#define kSystemGovt         10000
#define kSystemAllied       15000
#define kSystemNotGovt      20000
#define kSystemEnemy        25000
#define kSystemClass        30000
#define kSystemNotClass     31000
#define kSystemBandLimit    32000

/* briefing text buffer */
#define kBriefingTextMax    0x1ffe

/* strncpy of a briefing (kBriefingTextMax bytes) into an 8 KB text buffer, then a NUL */
inline void CopyBriefingText(char *dst, const char *src)
{
    strncpy(dst, src, kBriefingTextMax);
    dst[kBriefingTextMax] = 0;
}

/* negative pay bands */
#define kPayClearGovt   10000   /* pay -(10000 + id): clear the record with govt id */
#define kPayClearAllies 20000
#define kPayClearClass  30000
#define kPctOfCashBase  40000   /* pay -40001..-40099 is N% of the player's cash */

/* --- one declaration per name --- */
extern Boolean CronDateInRange(CronRecord *cron);           /* 0x0046c800 */
extern void CronEventHandler(void);                         /* 0x00439500 */
extern void DisasterHandler(void);                         /* 0x00424f90 */
extern Boolean DoMissionAccept(short mission);                    /* 0x0043f100 */
extern void DoMissionFailure(short slot);                   /* 0x00440930 */
extern void DoMissionSuccess(short slot);                 /* 0x00440410 */
extern std::string HighGoalAbbreviation(short state);        /* 0x0040db70 */
extern std::string LowGoalAbbreviation(short state);         /* 0x0040ddc0 */
extern void EvalCurrentMissionBitSetString(char *expr, short mission); /* 0x00448050 */
extern void EvalSetExp(void);     /* 0x00449370 */
extern short EvalTestExp(short *err);  /* 0x00449020 */
extern Boolean DoMissionCargoPickup(short cargo, short needed);        /* 0x00440370 cargo room */
extern void ApplyMissionPay(int32_t amount);  /* 0x00440750 */
extern void HandleComm(void);                  /* 0x0042f1b0 (gameloop.cpp; mac _HandleComm) */
extern void LoadCurrentMissionData(short mission, short slot);   /* 0x0043f8c0 */
extern void MissionHandler(void);                /* 0x00443760 */
void MissionLandCargoCheck(short slot, short stel);
extern void MissionLandCheck(void);                      /* 0x00443780 */
extern void MissionObjectivesCheck(short slot);           /* 0x00443c60 */
extern void MissionPostLoadValidCheck(void);     /* 0x0043c3e0 */
extern short OfferOneMission(short mission);                  /* 0x00442510 */
extern Boolean PlayerMeetsRequirements(uint32_t reqLo, uint32_t reqHi); /* 0x0046cd80 */
extern void QuickMissionFailure(short mission);                  /* 0x00440bf0 */
extern void RandomizeMissionData(void);                         /* 0x0043cf00 */
extern void RandomizeOneMission(short mission);          /* 0x0043d240 mac _RandomizeOneMission */
extern void RestoreBuffer(const std::string &saved);    /* 0x0044a9d0 */
extern short SelectMissionStellar(short sel, short loc, short avoid, short mission); /* 0x0043d510 */
extern short SelectMissionSystem(short which, short curSystem);  /* 0x0043e6f0 */
extern void SetupMissionList(void);                            /* 0x0043cc20 */
extern char commMsg[2048];                      /* 0x0073130c */
extern short currentMissionIndexForSetParser; /* 0x00776b00 (mac name, 2 uses both sides) */
extern short lastInteger;               /* 0x007caa10 mac _lastInteger */
extern Handle missionFileHand;                  /* 0x0077efb8 (mac _missionFileHand) */
extern unsigned char missionName[][256];        /* 0x007356e8 (mac _missionName) */
extern Boolean offeringFromShip;                    /* 0x00773eea (mac name) */
extern short offeringShipIndex;                    /* 0x0077430e (mac _offeringShipIndex) */

/* --- one declaration per name --- */
extern Boolean IsMissionAvailable(short mission, Boolean offer); /* 0x00441b40 */
extern MissionAvailRec missionAvailData[];              /* 0x0077f62c (corroborated, matched-globals.csv) */
extern short activeMissions[16];
extern ScrollRec **missionInfoList;
extern short tempMissions[];

#endif /* MISSIONS_H */
