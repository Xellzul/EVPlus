/* statusparts.cpp -- the status-panel painters at win link order 0x0045f530 (DrawStatusTarg), 0x00460ec0 (DrawStatusWeap) and 0x004612c0 (DrawStatusCargo); contiguous, and the next function is trigtables.cpp's, so the run is one original .obj. Notes: Research/notes/statusparts.md */

#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "stellar.h"
#include "prototypes.h"
#include "space.h"
#include "ai.h"
#include "game.h"
#include "missions.h"
#include "burgermath.h"
#include "text.h"

// FUNCTION: EVNOVA 0x0045e400
// MAC: _DrawStatusNav i386 0x00049f43 ppc 0x00052bcc
void DrawStatusNav(void)
{
    Rect dstRect;
    Rect srcRect;
    FloatPoint origin;
    float range;
    short i;
    short stellar;
    Boolean safe;
    short j;

    if (statusOffRect[kNavRect].left >= statusOffRect[kNavRect].right ||
        statusOffRect[kNavRect].top >= statusOffRect[kNavRect].bottom)
        return;
    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    srcRect = statusOffRect[kNavRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &srcRect, &dstRect, 0, 0);
    TextFont(statusFontID);
    TextSize(statusFontSize[0]);

    if (shipData[0].navTargetKind == -1) {
        RGBForeColor(statusDimColor);
        DrawStringCentered(IndString(kGameStrList, kNavNoDestination), dstRect.left, dstRect.right,
                           dstRect.top + 0x16);
    }
    if (shipData[0].navTargetKind == 2) {
        RGBForeColor(statusDimColor);
        DrawStringCentered(IndString(kGameStrList, kNavStellarLabel), dstRect.left, dstRect.right,
                           dstRect.top + 0xc);
        if (shipData[0].targetStellar != -1) {
            RGBForeColor(statusBrightColor);
            DrawStringCentered(SpobName(shipData[0].targetStellar), dstRect.left, dstRect.right,
                               dstRect.top + 0x1d);
        } else {
            RGBForeColor(statusDimColor);
            DrawStringCentered(IndString(kGameStrList, kNavUnknown), dstRect.left, dstRect.right,
                               dstRect.top + 0x1d);
        }
    }
    if (shipData[0].navTargetKind == 3) {
        if (shipData[0].jumpTimer > 0.0f)
            RGBForeColor(statusBrightColor);
        else
            RGBForeColor(statusDimColor);
        DrawStringCentered(IndString(kGameStrList, kNavHyperLabel), dstRect.left, dstRect.right,
                           dstRect.top + 0xc);
        if (shipData[0].targetStellar != -1) {
            origin.x = origin.y = 0.0f;
            safe = 1;
            range = shipData->HyperSafeDist();
            for (j = 0; j < kNavSlots; j++) {
                stellar = systemData[shipData[0].curSystem].navDefault[j];
                if (stellar != -1 && (stellarData[stellar].flags2 & kSpobGateOrWormhole) == 0) {
                    origin.x = origin.y = 0.0f;
                    range = shipData->HyperSafeDist();
                    if (FloatAbs(XYFloatSquaredDist(&origin,
                                                     &shipData[0].pos)) <= range) {
                        safe = 0;
                        break;
                    }
                }
            }
            if ((safe != 0 && shipData[0].fuel >= kHyperFuelUnit) || shipData[0].jumpTimer > 0.0f)
                RGBForeColor(statusBrightColor);
            else
                RGBForeColor(statusDimColor);
            i = systemData[shipData[0].curSystem].links[shipData[0].targetStellar];
            if (systemData[i].exploreLevel > 0)
                DrawStringCentered(SystName(i), dstRect.left, dstRect.right, dstRect.top + 0x1d);
            else
                DrawStringCentered(IndString(kGameStrList, kNavUnexplored), dstRect.left, dstRect.right,
                                   dstRect.top + 0x1d);
        } else {
            RGBForeColor(statusDimColor);
            DrawStringCentered(IndString(kGameStrList, kNavUnknown), dstRect.left, dstRect.right,
                               dstRect.top + 0x1d);
        }
    }

    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (preventStatusCopy == 0)
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &dstRect, &dstRect, 0, 0);
}

/* the shield bar is statusOffRect[1], the armour bar statusOffRect[2] */
#define kShieldRect  1
#define kArmorRect   2
/* the fuel bar is statusOffRect[3] */
#define kFuelRect    3

// FUNCTION: EVNOVA 0x0045e9c0
// MAC: _DrawStatusShield i386 0x0004a452 ppc 0x000530b8
void DrawStatusShield(void)
{
    Rect dstRect;
    Rect srcRect;
    Rect barRect;
    float level;
    float cap;

    SetGWorldContainer(&loadPort);
    if (statusOffRect[kShieldRect].right > statusOffRect[kShieldRect].left &&
        statusOffRect[kShieldRect].bottom > statusOffRect[kShieldRect].top) {
        CopyRect(&srcRect, &statusOffRect[kShieldRect]);
        CopyRect(&dstRect, &srcRect);
        OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
        CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
                 &srcRect, &dstRect, 0, 0);
        if (shipData[0].shield > 0.0f) {
            float cap;
            float level;
            CopyRect(&barRect, &dstRect);
            cap = shipData->ShieldCapacity();
            level = shipData[0].shield;
            if (level <= cap) {
                if (dstRect.right - dstRect.left > dstRect.bottom - dstRect.top) {
                    barRect.right = barRect.left + (dstRect.right - dstRect.left) * (level / cap);
                    if (barRect.right > dstRect.right)
                        barRect.right = dstRect.right;
                } else {
                    barRect.top = barRect.bottom - (dstRect.bottom - dstRect.top) * (level / cap);
                    if (barRect.top < dstRect.top)
                        barRect.top = dstRect.top;
                }
            }
            RGBForeColor(shieldStatusColor);
            PaintRect(&barRect);
            ForeColor(blackColor);
        }
        ForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
        if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
            CopyRect(&barRect, &dstRect);
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &dstRect, &barRect, 0, 0);
        }
    }

    SetGWorldContainer(&loadPort);
    if (statusOffRect[kArmorRect].right > statusOffRect[kArmorRect].left &&
        statusOffRect[kArmorRect].bottom > statusOffRect[kArmorRect].top) {
        CopyRect(&srcRect, &statusOffRect[kArmorRect]);
        CopyRect(&dstRect, &srcRect);
        OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
        CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
                 &srcRect, &dstRect, 0, 0);
        if (shipData[0].armor > 0.0f) {
            CopyRect(&barRect, &dstRect);
            cap = shipData->ArmorCapacity();
            level = shipData[0].armor;
            if (level <= cap) {
                if (dstRect.right - dstRect.left > dstRect.bottom - dstRect.top) {
                    barRect.right = barRect.left + (dstRect.right - dstRect.left) * (level / cap);
                    if (barRect.right > dstRect.right)
                        barRect.right = dstRect.right;
                } else {
                    barRect.top = barRect.bottom - (dstRect.bottom - dstRect.top) * (level / cap);
                    if (barRect.top < dstRect.top)
                        barRect.top = dstRect.top;
                }
            }
            RGBForeColor(armorStatusColor);
            PaintRect(&barRect);
            ForeColor(blackColor);
        }
        ForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
        if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
            CopyRect(&barRect, &dstRect);
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &dstRect, &barRect, 0, 0);
        }
    }
}

// FUNCTION: EVNOVA 0x0045efe0
// MAC: _DrawStatusFuel i386 0x0004a8ff ppc 0x0005366c
void DrawStatusFuel(void)
{
    Rect dstRect;
    Rect srcRect;
    Rect barRect;
    Rect jumpRect;
    short jumps;
    short partial;

    if (statusOffRect[kFuelRect].left >= statusOffRect[kFuelRect].right ||
        statusOffRect[kFuelRect].top >= statusOffRect[kFuelRect].bottom)
        return;
    SetGWorldContainer(&loadPort);
    CopyRect(&srcRect, &statusOffRect[kFuelRect]);
    CopyRect(&dstRect, &srcRect);
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &srcRect, &dstRect, 0, 0);
    if (shipData[0].fuel > 0.0f && shipData->IsDisabled() == 0) {
        RGBForeColor(fuelStatusColor[0]);
        CopyRect(&barRect, &dstRect);
        if (srcRect.right - srcRect.left > srcRect.bottom - srcRect.top) {
            barRect.right = barRect.left + (dstRect.right - dstRect.left) *
                                    (shipData[0].fuel / shipData->FuelCapacity());
            if (barRect.right > dstRect.right)
                barRect.right = dstRect.right;
        } else {
            barRect.top = barRect.bottom - (dstRect.bottom - dstRect.top) *
                                  (shipData[0].fuel / shipData->FuelCapacity());
            if (barRect.top < dstRect.top)
                barRect.top = dstRect.top;
        }
        PaintRect(&barRect);
        jumps = (shipData[0].fuel / kHyperFuelUnit);
        partial = (shipData[0].fuel - jumps);
        if (partial > 0) {
            CopyRect(&jumpRect, &barRect);
            if (srcRect.right - srcRect.left > srcRect.bottom - srcRect.top) {
                jumpRect.left += (dstRect.right - dstRect.left) *
                                 (jumps * 100 / (float)shipData->FuelCapacity());
            } else {
                jumpRect.bottom -= (dstRect.bottom - dstRect.top) *
                                   (jumps * 100 / (float)shipData->FuelCapacity());
            }
            RGBForeColor(fuelStatusColor[1]);
            PaintRect(&jumpRect);
        }
    }
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
        CopyRect(&barRect, &dstRect);
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &dstRect, &barRect, 0, 0);
    }
}

// FUNCTION: EVNOVA 0x0045f530
// MAC: _DrawStatusTarg i386 0x0004ace1 ppc 0x00053ba4
void DrawStatusTarg(void)
{
    Boolean found;
    std::string label;
    std::string subtitle;
    Rect dstRect;
    Rect srcRect;
    short mission;
    double capacity;
    Boolean vulnerable;
    short orders;
    short h;
    short v;
    Rect picRect;

    if (statusOffRect[kTargRect].left >= statusOffRect[kTargRect].right ||
        statusOffRect[kTargRect].top >= statusOffRect[kTargRect].bottom)
        return;

    if (shipData[0].targetShip != -1) {
        lastTargetedShipShields = shipData[shipData[0].targetShip].shield;
        lastTargetedShipClass = shipData[shipData[0].targetShip].type;
    } else {
        lastTargetedShipShields = -1;
        lastTargetedShipClass = -1;
    }
    lastDebugTargInfoState = targKeyOnNow;

    SetGWorldContainer(&loadPort);
    TextFont(statusFontID);
    TextSize(statusFontSize[0]);

    srcRect = statusOffRect[kTargRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    ForeColor(blackColor);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &srcRect, &dstRect, 0, 0);

    if (targKeyOnNow != 0 && cheatsActive != 0) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        if (shipData[0].targetShip == -1)
            shipData[0].targetShip = 0;

        MoveTo(dstRect.right - 15, dstRect.top + 20);
        RGBForeColor(statusDimColor);
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[0].targetShip));

        MoveTo(dstRect.left + 5, dstRect.top + 20);
        RGBForeColor(statusDimColor);
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].index));

        MoveTo(dstRect.left + 5, dstRect.top + 40);
        RGBForeColor(statusDimColor);
        DrawCString("hg: ");
        RGBForeColor(statusBrightColor);
        DrawHighGoalAbbreviation(shipData[shipData[0].targetShip].aiState);

        MoveTo(dstRect.left + 70, dstRect.top + 40);
        RGBForeColor(statusDimColor);
        DrawCString("lg: ");
        RGBForeColor(statusBrightColor);
        DrawLowGoalAbbreviation(shipData[shipData[0].targetShip].aiSubstate);

        MoveTo(dstRect.left + 5, dstRect.top + 55);
        RGBForeColor(statusDimColor);
        DrawCString("tnav: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].targetStellar));

        MoveTo(dstRect.left + 70, dstRect.top + 55);
        RGBForeColor(statusDimColor);
        DrawCString("twep: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].targetShip));

        MoveTo(dstRect.left + 5, dstRect.top + 70);
        RGBForeColor(statusDimColor);
        DrawCString("cnt: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].aiTimer));

        MoveTo(dstRect.left + 70, dstRect.top + 70);
        RGBForeColor(statusDimColor);
        DrawCString("wp: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].selectedWeapon));

        MoveTo(dstRect.left + 5, dstRect.top + 85);
        RGBForeColor(statusDimColor);
        DrawCString("swl: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(AIFindSwarmLeader(&shipData[shipData[0].targetShip])));

        MoveTo(dstRect.left + 55, dstRect.top + 85);
        RGBForeColor(statusDimColor);
        DrawCString("ai: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].aiType));
        if (shipData[shipData[0].targetShip].defenseStellar != -1) {
            DrawCString("d");
        } else {
            if ((shipClass[shipData[shipData[0].targetShip].type].flags3 & kShipMiner) != 0 &&
                shipData[shipData[0].targetShip].parentShip == -1) {
                DrawCString("m");
            } else if (shipData[shipData[0].targetShip].aiType == kAIWarship && shipData[shipData[0].targetShip].govt != -1 &&
                       (govtData[shipData[shipData[0].targetShip].govt].flags & kGovtPlunders) != 0) {
                DrawCString("p");
            }
        }

        MoveTo(dstRect.left + 120, dstRect.top + 85);
        RGBForeColor(statusDimColor);
        DrawCString("fl: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].formLeader));

        MoveTo(dstRect.left + 150, dstRect.top + 85);
        RGBForeColor(statusDimColor);
        DrawCString("sl: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].swarmLeader));

        if (shipData[shipData[0].targetShip].parentShip != -1) {
            MoveTo(dstRect.left + 85, dstRect.top + 85);
            if (shipData[shipData[0].targetShip].parentShip == 0)
                ForeColor(cyanColor);
            else
                RGBForeColor(statusDimColor);
            DrawCString("or: ");
            RGBForeColor(statusBrightColor);
            orders = shipData[shipData[0].targetShip].escortOrder;
            if ((int)orders == 0)
                DrawCString("form");
            else if ((int)orders == 1)
                DrawCString("def");
            else if ((int)orders == 2)
                DrawCString("atk");
            else if ((int)orders == 4)
                DrawCString("wait");
            else if ((int)orders == 3)
                DrawCString("ret");
            else {
                DrawCString(NumText(orders));
            }
        }

        MoveTo(dstRect.left + 45, dstRect.top + 100);
        RGBForeColor(statusBrightColor);
        if (shipData[shipData[0].targetShip].defenseStellar != -1) {
            DrawCString("Df ");
        } else {
            if (shipData[shipData[0].targetShip].specialMission != -1)
                DrawCString("Sp");
        }

        MoveTo(dstRect.left + 45, dstRect.top + 100);
        RGBForeColor(statusDimColor);
        DrawCString("gov: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].govt));

        MoveTo(dstRect.left + 5, dstRect.top + 100);
        RGBForeColor(statusDimColor);
        DrawCString("t: ");
        RGBForeColor(statusBrightColor);
        DrawCString(NumText(shipData[shipData[0].targetShip].type + 128));

        if (shipData[0].targetShip == 0)
            shipData[0].targetShip = -1;
    } else if (shipData[0].targetShip != -1) {
        RGBForeColor(statusBrightColor);

        found = 0;
        if (shipData[shipData[0].targetShip].specialMission != -1) {
            if (MissionHasShipName(shipData[shipData[0].targetShip].specialMission))
                found = 1;
        }
        if (found) {
            DrawStringCentered(MissionShipName(shipData[shipData[0].targetShip].specialMission),
                               dstRect.left, dstRect.right, dstRect.top + 16);
        } else {
            if (shipData[shipData[0].targetShip].persType != -1)
                DrawStringCentered(PersonName(shipData[shipData[0].targetShip].persType), dstRect.left, dstRect.right,
                                   dstRect.top + 16);
            else
                DrawStringCentered(ShipClassName(shipData[shipData[0].targetShip].type), dstRect.left,
                                   dstRect.right, dstRect.top + 16);
        }

        h = (dstRect.left + dstRect.right) / 2;
        v = (dstRect.top + dstRect.bottom) / 2;
        SetRect(&picRect, h - 64, v - 32, h + 64, v + 32);
        ForeColor(blackColor);
        h = shipClass[shipData[shipData[0].targetShip].type].targetPictID;
        if (targetInfoPic[h] != NULL)
            DrawPicHandle((PicHandle)targetInfoPic[h], &picRect);

        if (shipData[shipData[0].targetShip].specialMission != -1) {
            if (MissionHasShipSubtitle(shipData[shipData[0].targetShip].specialMission))
                subtitle = MissionShipSubtitle(shipData[shipData[0].targetShip].specialMission);
            else
                subtitle = FromNameField(shipClass[shipData[shipData[0].targetShip].type].subtitle);
        } else {
            if (shipData[shipData[0].targetShip].persType != -1) {
                if (personData[shipData[shipData[0].targetShip].persType].subtitle[0] > 0)
                    subtitle = FromNameField(personData[shipData[shipData[0].targetShip].persType].subtitle);
                else
                    subtitle = FromNameField(shipClass[shipData[shipData[0].targetShip].type].subtitle);
            } else {
                subtitle = FromNameField(shipClass[shipData[shipData[0].targetShip].type].subtitle);
            }
        }
        TextSize(statusFontSize[1]);
        RGBForeColor(statusBrightColor);
        DrawStringCentered(subtitle, dstRect.left, dstRect.right, dstRect.top + 29);
        TextSize(statusFontSize[0]);
        ForeColor(blackColor);

        if ((shipClass[shipData[shipData[0].targetShip].type].flags & kShipHideStatus) != 0) {
            if (shipData[shipData[0].targetShip].govt >= 0) {
                RGBForeColor(statusDimColor);
                MoveTo(dstRect.left + 87, dstRect.bottom - 6);
                DrawStringCentered(govtTargStrings[shipData[shipData[0].targetShip].govt], dstRect.left,
                                   dstRect.right, dstRect.bottom - 6);
            }
        } else {
            found = 0;
            if (shipData[shipData[0].targetShip].persType != -1 &&
                personData[shipData[shipData[0].targetShip].persType].shieldMod < 0.0f)
                found = 1;
            if (found) {
                if (shipData[shipData[0].targetShip].govt >= 0) {
                    RGBForeColor(statusDimColor);
                    MoveTo(dstRect.left + 87, dstRect.bottom - 6);
                    DrawStringCentered(govtTargStrings[shipData[shipData[0].targetShip].govt], dstRect.left,
                                       dstRect.right, dstRect.bottom - 6);
                }
            } else {
            MoveTo(dstRect.left + 5, dstRect.bottom - 6);
            if (shipData[shipData[0].targetShip].shield <= 0.0f || shipData[shipData[0].targetShip].IsDisabled() != 0) {
                RGBForeColor(statusBrightColor);
                MoveTo(dstRect.left + 5, dstRect.bottom - 6);
                if (shipData[shipData[0].targetShip].IsDisabled() != 0) {
                    vulnerable = 0;
                    mission = shipData[shipData[0].targetShip].specialMission;
                    if (mission != -1) {
                        if (currentMissionObjectives[mission].active != 0 &&
                            currentMissions[mission].shipGoal == kMissionGoalRescue &&
                            currentMissionObjectives[mission].objectivesDone == 0 &&
                            shipData[shipData[0].targetShip].boarded == 0) {
                            if ((shipClass[shipData[shipData[0].targetShip].type].flags & kShipArmorDisableTenth) != 0) {
                                if (shipData[shipData[0].targetShip].armor * 100.0f >=
                                        shipData[shipData[0].targetShip].ArmorCapacity() * 10.0)
                                    vulnerable = 1;
                            } else {
                                if (shipData[shipData[0].targetShip].armor * 100.0f >=
                                        shipData[shipData[0].targetShip].ArmorCapacity() * 33.333)
                                    vulnerable = 1;
                            }
                        }
                    }
                    if (vulnerable)
                        DrawIndString(kGameStrList, kStatusVulnerable);
                    else
                        DrawIndString(kGameStrList, kStatusDisabled);
                } else {
                    if ((shipClass[shipData[shipData[0].targetShip].type].flags & kShipShowArmorPercent) != 0) {
                        RGBForeColor(statusDimColor);
                        DrawCString(armorString[0]);
                        DrawCString(" ");
                        RGBForeColor(statusBrightColor);
                        capacity = shipData[shipData[0].targetShip].ArmorCapacity();
                        if (capacity > 0.0) {
                            if (shipData[shipData[0].targetShip].armor <= 0.0f) {
                                DrawCString("0%");
                            } else {
                                if (shipData[shipData[0].targetShip].armor <= capacity) {
                                    DrawCString(NumText(shipData[shipData[0].targetShip].armor * 100.0f / capacity));
                                    DrawCString("%");
                                } else {
                                    DrawCString("100%");
                                }
                            }
                        } else {
                            DrawIndString(kGameStrList, kStatusDestroyed);
                        }
                    } else {
                        if (shipClass[shipData[shipData[0].targetShip].type].shield <= 0)
                            DrawCString(shieldString[3]);
                        else
                            DrawCString(shieldString[4]);
                    }
                }
            } else {
                RGBForeColor(statusDimColor);
                DrawCString(shieldString[2]);
                DrawCString(" ");
                RGBForeColor(statusBrightColor);
                capacity = shipData[shipData[0].targetShip].ShieldCapacity();
                if (shipData[shipData[0].targetShip].shield <= capacity) {
                    DrawCString(NumText((shipData[shipData[0].targetShip].shield / capacity * 100.0)));
                    DrawCString("%");
                } else {
                    DrawCString("100%");
                }
            }

            if (shipData[shipData[0].targetShip].govt >= 0) {
                RGBForeColor(statusDimColor);
                MoveTo(dstRect.right - 7 - MeasureCString(govtTargStrings[shipData[shipData[0].targetShip].govt]),
                       dstRect.bottom - 6);
                DrawCString(govtTargStrings[shipData[shipData[0].targetShip].govt]);
            } else {
                if (shipData[shipData[0].targetShip].parentShip == 0 || shipData[shipData[0].targetShip].formerEscort >= 0) {
                    if (shipData[shipData[0].targetShip].aiType == kAIFighter || shipData[shipData[0].targetShip].formerEscort == 0) {
                        if (shipClass[shipData[shipData[0].targetShip].type].mass >= 100)
                            label = IndString(kGameStrList, kStatusNeutral);
                        else
                            label = IndString(kGameStrList, kStatusHostile);
                    } else {
                        label = IndString(kGameStrList, kStatusNeutral);
                    }
                    RGBForeColor(statusDimColor);
                    MoveTo(dstRect.right - 7 - MeasureCString(label), dstRect.bottom - 6);
                    DrawCString(label);
                }
            }
            }
        }
    } else {
        RGBForeColor(statusDimColor);
        DrawStringCentered(IndString(kGameStrList, kStatusNoTarget), dstRect.left, dstRect.right, dstRect.top + 47);
    }

    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    if (preventStatusCopy == 0)
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &dstRect, &dstRect, 0, 0);
}

// FUNCTION: EVNOVA 0x00460ec0
// MAC: _DrawStatusWeap i386 0x0004c00a ppc 0x00054e90
void DrawStatusWeap(void)
{
    std::string text;
    Rect dstRect;
    Rect srcRect;

    if (statusOffRect[kWeapRect].left >= statusOffRect[kWeapRect].right ||
        statusOffRect[kWeapRect].top >= statusOffRect[kWeapRect].bottom)
        return;
    SetGWorldContainer(&loadPort);
    TextFont(statusFontID);
    TextSize(statusFontSize[0]);
    srcRect = statusOffRect[kWeapRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &srcRect, &dstRect, 0, 0);
    if (shipData[0].selectedWeapon == -1) {
        RGBForeColor(statusDimColor);
        DrawStringCentered(IndString(kGameStrList, kStatusNoWeapon), dstRect.left, dstRect.right,
                           dstRect.top + 0xc);
    } else {
        RGBForeColor(statusBrightColor);
        if (weaponStats[shipData[0].selectedWeapon].ammoType == -1 ||
            (weaponStats[shipData[0].selectedWeapon].flags2 & kWeapNoAmmoDisplay) != 0) {
            DrawStringCentered(weapName[shipData[0].selectedWeapon], dstRect.left,
                               dstRect.right, dstRect.top + 0xc);
        } else if (weaponStats[shipData[0].selectedWeapon].ammoType > -kWeapAmmoFuelBase) {
            text = UpToNul(weapName[shipData[0].selectedWeapon]);
            text += " - ";
            if (weaponStats[shipData[0].selectedWeapon].ammoType >= 0 &&
                weaponStats[shipData[0].selectedWeapon].ammoType < 256 &&
                weaponStats[shipData[0].selectedWeapon].guidance != kGuidanceBay)
                text += NumText(shipData[0].weap[weaponStats[shipData[0].selectedWeapon].ammoType].ammo);
            else
                text += NumText(shipData[0].weap[shipData[0].selectedWeapon].ammo);
            DrawStringCentered(text, dstRect.left, dstRect.right, dstRect.top + 0xc);
        } else if (weaponStats[shipData[0].selectedWeapon].ammoType <= -kWeapAmmoFuelBase) {
            DrawStringCentered(weapName[shipData[0].selectedWeapon], dstRect.left,
                               dstRect.right, dstRect.top + 0xc);
        }
    }
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (preventStatusCopy == 0) {
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &dstRect, &dstRect, 0, 0);
    }
}

// FUNCTION: EVNOVA 0x004612c0
// MAC: _DrawStatusCargo i386 0x0004c313 ppc 0x00055174
void DrawStatusCargo(void)
{
    Rect dstRect;
    std::string text;
    Rect srcRect;
    short i;
    short j;
    short missionCargoCount;
    short junkCount;
    short missionCargoType;
    short junkIndex;

    if (statusOffRect[kCargoRect].left >= statusOffRect[kCargoRect].right ||
        statusOffRect[kCargoRect].top >= statusOffRect[kCargoRect].bottom)
        return;
    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    TextFont(statusFontID);
    TextSize(statusFontSize[0]);
    srcRect = statusOffRect[kCargoRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &srcRect, &dstRect, 0, 0);
    for (i = 0; i <= 5; i++) {
        if (shipData[0].cargo[i] > 0) {
            RGBForeColor(statusDimColor);
            MoveTo(dstRect.left + 3, dstRect.top + (i + 1) * 14 - 2);
            DrawCString(statusCargoName[i]);
            RGBForeColor(statusBrightColor);
            MoveTo(dstRect.left + 0x29, dstRect.top + (i + 1) * 14 - 2);
            DrawCString(NumText(shipData[0].cargo[i]));
        }
    }
    MoveTo(dstRect.left + 0x4d, dstRect.top + 0xc);
    RGBForeColor(statusDimColor);
    DrawIndString(kGameStrList, 0x13);
    MoveTo(dstRect.left + 0x6e, dstRect.top + 0xc);
    RGBForeColor(statusBrightColor);
    missionCargoCount = TotalFleetHolds() - TotalCargo(shipData);
    if (missionCargoCount < 0)
        missionCargoCount = 0;
    DrawCString(NumText(missionCargoCount));
    missionCargoCount = 0;
    junkCount = 0;
    missionCargoType = -1;
    junkIndex = -1;
    for (j = 0; j < 16; j++) {
        if (currentMissionObjectives[j].active != 0 && currentMissions[j].hasCargo != 0 &&
            currentMissions[j].cargoType != -1) {
            if (missionCargoCount == 0)
                missionCargoType = currentMissions[j].cargoType;
            missionCargoCount++;
        }
    }
    for (i = 0; i < 128; i++) {
        if (junkData[i].held > 0) {
            if (junkCount == 0)
                junkIndex = i;
            junkCount++;
        }
    }
    if (missionCargoCount > 0 || junkCount > 0) {
        MoveTo(dstRect.left + 0x4d, dstRect.top + 0x1e);
        RGBForeColor(statusDimColor);
        DrawIndString(kGameStrList, 0x14);
        MoveTo(dstRect.left + 0x57, dstRect.top + 0x2e);
        RGBForeColor(statusBrightColor);
        if (missionCargoCount == 1) {
            DrawCString(TableEntry(cargoAbbrev, missionCargoType));
        } else {
            if (junkCount == 1) {
                DrawCString(FromField(junkData[junkIndex].abbrev, sizeof junkData[junkIndex].abbrev));
            } else {
                DrawIndString(kGameStrList, 0x15);
            }
        }
    }
    MoveTo(dstRect.left + 0x4d, dstRect.top + 0x42);
    RGBForeColor(statusDimColor);
    text = creditsString;
    if (!text.empty())
        text[0] = (char)toupper((unsigned char)text[0]);
    DrawCString(text);
    DrawCString(":");
    MoveTo(dstRect.left + 0x57, dstRect.top + 0x52);
    RGBForeColor(statusBrightColor);
    DrawCommaNumber(shipData[0].credits);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (preventStatusCopy == 0) {
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &dstRect, &dstRect, 0, 0);
    }
}
