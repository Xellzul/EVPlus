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
#include "painter.h"

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
    UI::Painter load(UI::Surface::Load());
    const UI::TextSpec dim = { UI::Style::Status, *statusDimColor };
    const UI::TextSpec bright = { UI::Style::Status, *statusBrightColor };

    srcRect = statusOffRect[kNavRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    load.Copy(StatusPicture(), srcRect, dstRect);

    if (shipData[0].navTargetKind == -1) {
        load.Text(dstRect.left, dstRect.right, dstRect.top + 0x16, IndString(kGameStrList, kNavNoDestination), dim,
                  UI::Align::Center);
    }
    if (shipData[0].navTargetKind == 2) {
        load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, IndString(kGameStrList, kNavStellarLabel), dim,
                  UI::Align::Center);
        if (shipData[0].targetStellar != -1) {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0x1d, SpobName(shipData[0].targetStellar), bright,
                      UI::Align::Center);
        } else {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0x1d, IndString(kGameStrList, kNavUnknown), dim,
                      UI::Align::Center);
        }
    }
    if (shipData[0].navTargetKind == 3) {
        load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, IndString(kGameStrList, kNavHyperLabel),
                  shipData[0].jumpTimer > 0.0f ? bright : dim, UI::Align::Center);
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
            const UI::TextSpec &ready =
                (safe != 0 && shipData[0].fuel >= kHyperFuelUnit) || shipData[0].jumpTimer > 0.0f ? bright : dim;
            i = systemData[shipData[0].curSystem].links[shipData[0].targetStellar];
            if (systemData[i].exploreLevel > 0)
                load.Text(dstRect.left, dstRect.right, dstRect.top + 0x1d, SystName(i), ready, UI::Align::Center);
            else
                load.Text(dstRect.left, dstRect.right, dstRect.top + 0x1d, IndString(kGameStrList, kNavUnexplored),
                          ready, UI::Align::Center);
        } else {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0x1d, IndString(kGameStrList, kNavUnknown), dim,
                      UI::Align::Center);
        }
    }

    if (preventStatusCopy == 0)
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, dstRect);
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

    UI::Painter load(UI::Surface::Load());

    if (statusOffRect[kShieldRect].right > statusOffRect[kShieldRect].left &&
        statusOffRect[kShieldRect].bottom > statusOffRect[kShieldRect].top) {
        CopyRect(&srcRect, &statusOffRect[kShieldRect]);
        CopyRect(&dstRect, &srcRect);
        OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
        load.Copy(StatusPicture(), srcRect, dstRect);
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
            load.Fill(barRect, *shieldStatusColor);
        }
        if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
            CopyRect(&barRect, &dstRect);
            UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, barRect);
        }
    }

    if (statusOffRect[kArmorRect].right > statusOffRect[kArmorRect].left &&
        statusOffRect[kArmorRect].bottom > statusOffRect[kArmorRect].top) {
        CopyRect(&srcRect, &statusOffRect[kArmorRect]);
        CopyRect(&dstRect, &srcRect);
        OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
        load.Copy(StatusPicture(), srcRect, dstRect);
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
            load.Fill(barRect, *armorStatusColor);
        }
        if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
            CopyRect(&barRect, &dstRect);
            UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, barRect);
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
    UI::Painter load(UI::Surface::Load());

    CopyRect(&srcRect, &statusOffRect[kFuelRect]);
    CopyRect(&dstRect, &srcRect);
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    load.Copy(StatusPicture(), srcRect, dstRect);
    if (shipData[0].fuel > 0.0f && shipData->IsDisabled() == 0) {
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
        load.Fill(barRect, *fuelStatusColor[0]);
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
            load.Fill(jumpRect, *fuelStatusColor[1]);
        }
    }
    if (gSpriteWorld->fastBlit != 0 && preventStatusCopy == 0) {
        CopyRect(&barRect, &dstRect);
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, barRect);
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

    UI::Painter load(UI::Surface::Load());
    const UI::TextSpec dim = { UI::Style::Status, *statusDimColor };
    const UI::TextSpec bright = { UI::Style::Status, *statusBrightColor };

    srcRect = statusOffRect[kTargRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    load.Copy(StatusPicture(), srcRect, dstRect);

    if (targKeyOnNow != 0 && cheatsActive != 0) {
        const UI::TextSpec dimSmall = { UI::Style::Small, *statusDimColor };
        const UI::TextSpec brightSmall = { UI::Style::Small, *statusBrightColor };

        if (shipData[0].targetShip == -1)
            shipData[0].targetShip = 0;

        load.Text(dstRect.right - 15, dstRect.top + 20, NumText(shipData[0].targetShip), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 20, NumText(shipData[shipData[0].targetShip].index), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 40, "hg: ", dimSmall);
        load.Text(HighGoalAbbreviation(shipData[shipData[0].targetShip].aiState), brightSmall);

        load.Text(dstRect.left + 70, dstRect.top + 40, "lg: ", dimSmall);
        load.Text(LowGoalAbbreviation(shipData[shipData[0].targetShip].aiSubstate), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 55, "tnav: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].targetStellar), brightSmall);

        load.Text(dstRect.left + 70, dstRect.top + 55, "twep: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].targetShip), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 70, "cnt: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].aiTimer), brightSmall);

        load.Text(dstRect.left + 70, dstRect.top + 70, "wp: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].selectedWeapon), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 85, "swl: ", dimSmall);
        load.Text(NumText(AIFindSwarmLeader(&shipData[shipData[0].targetShip])), brightSmall);

        load.Text(dstRect.left + 55, dstRect.top + 85, "ai: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].aiType), brightSmall);
        if (shipData[shipData[0].targetShip].defenseStellar != -1) {
            load.Text("d", brightSmall);
        } else {
            if ((shipClass[shipData[shipData[0].targetShip].type].flags3 & kShipMiner) != 0 &&
                shipData[shipData[0].targetShip].parentShip == -1) {
                load.Text("m", brightSmall);
            } else if (shipData[shipData[0].targetShip].aiType == kAIWarship && shipData[shipData[0].targetShip].govt != -1 &&
                       (govtData[shipData[shipData[0].targetShip].govt].flags & kGovtPlunders) != 0) {
                load.Text("p", brightSmall);
            }
        }

        load.Text(dstRect.left + 120, dstRect.top + 85, "fl: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].formLeader), brightSmall);

        load.Text(dstRect.left + 150, dstRect.top + 85, "sl: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].swarmLeader), brightSmall);

        if (shipData[shipData[0].targetShip].parentShip != -1) {
            if (shipData[shipData[0].targetShip].parentShip == 0)
                load.Text(dstRect.left + 85, dstRect.top + 85, "or: ", { UI::Style::Small, *cyanColor });
            else
                load.Text(dstRect.left + 85, dstRect.top + 85, "or: ", dimSmall);
            orders = shipData[shipData[0].targetShip].escortOrder;
            if ((int)orders == 0)
                load.Text("form", brightSmall);
            else if ((int)orders == 1)
                load.Text("def", brightSmall);
            else if ((int)orders == 2)
                load.Text("atk", brightSmall);
            else if ((int)orders == 4)
                load.Text("wait", brightSmall);
            else if ((int)orders == 3)
                load.Text("ret", brightSmall);
            else {
                load.Text(NumText(orders), brightSmall);
            }
        }

        if (shipData[shipData[0].targetShip].defenseStellar != -1) {
            load.Text(dstRect.left + 45, dstRect.top + 100, "Df ", brightSmall);
        } else {
            if (shipData[shipData[0].targetShip].specialMission != -1)
                load.Text(dstRect.left + 45, dstRect.top + 100, "Sp", brightSmall);
        }

        load.Text(dstRect.left + 45, dstRect.top + 100, "gov: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].govt), brightSmall);

        load.Text(dstRect.left + 5, dstRect.top + 100, "t: ", dimSmall);
        load.Text(NumText(shipData[shipData[0].targetShip].type + 128), brightSmall);

        if (shipData[0].targetShip == 0)
            shipData[0].targetShip = -1;
    } else if (shipData[0].targetShip != -1) {
        found = 0;
        if (shipData[shipData[0].targetShip].specialMission != -1) {
            if (MissionHasShipName(shipData[shipData[0].targetShip].specialMission))
                found = 1;
        }
        if (found) {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 16,
                      MissionShipName(shipData[shipData[0].targetShip].specialMission), bright, UI::Align::Center);
        } else {
            if (shipData[shipData[0].targetShip].persType != -1)
                load.Text(dstRect.left, dstRect.right, dstRect.top + 16,
                          PersonName(shipData[shipData[0].targetShip].persType), bright, UI::Align::Center);
            else
                load.Text(dstRect.left, dstRect.right, dstRect.top + 16,
                          ShipClassName(shipData[shipData[0].targetShip].type), bright, UI::Align::Center);
        }

        h = (dstRect.left + dstRect.right) / 2;
        v = (dstRect.top + dstRect.bottom) / 2;
        SetRect(&picRect, h - 64, v - 32, h + 64, v + 32);
        h = shipClass[shipData[shipData[0].targetShip].type].targetPictID;
        if (targetInfoPic[h] != NULL)
            load.Picture((PicHandle)targetInfoPic[h], picRect);

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
        load.Text(dstRect.left, dstRect.right, dstRect.top + 29, subtitle,
                  { UI::Style::StatusLarge, *statusBrightColor }, UI::Align::Center);

        if ((shipClass[shipData[shipData[0].targetShip].type].flags & kShipHideStatus) != 0) {
            if (shipData[shipData[0].targetShip].govt >= 0) {
                load.Text(dstRect.left, dstRect.right, dstRect.bottom - 6,
                          govtTargStrings[shipData[shipData[0].targetShip].govt], dim, UI::Align::Center);
            }
        } else {
            found = 0;
            if (shipData[shipData[0].targetShip].persType != -1 &&
                personData[shipData[shipData[0].targetShip].persType].shieldMod < 0.0f)
                found = 1;
            if (found) {
                if (shipData[shipData[0].targetShip].govt >= 0) {
                    load.Text(dstRect.left, dstRect.right, dstRect.bottom - 6,
                              govtTargStrings[shipData[shipData[0].targetShip].govt], dim, UI::Align::Center);
                }
            } else {
            if (shipData[shipData[0].targetShip].shield <= 0.0f || shipData[shipData[0].targetShip].IsDisabled() != 0) {
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
                        load.Text(dstRect.left + 5, dstRect.bottom - 6, IndStringRaw(kGameStrList, kStatusVulnerable),
                                  bright);
                    else
                        load.Text(dstRect.left + 5, dstRect.bottom - 6, IndStringRaw(kGameStrList, kStatusDisabled),
                                  bright);
                } else {
                    if ((shipClass[shipData[shipData[0].targetShip].type].flags & kShipShowArmorPercent) != 0) {
                        load.Text(dstRect.left + 5, dstRect.bottom - 6, armorString[0], dim);
                        load.Text(" ", dim);
                        capacity = shipData[shipData[0].targetShip].ArmorCapacity();
                        if (capacity > 0.0) {
                            if (shipData[shipData[0].targetShip].armor <= 0.0f) {
                                load.Text("0%", bright);
                            } else {
                                if (shipData[shipData[0].targetShip].armor <= capacity) {
                                    load.Text(NumText(shipData[shipData[0].targetShip].armor * 100.0f / capacity),
                                              bright);
                                    load.Text("%", bright);
                                } else {
                                    load.Text("100%", bright);
                                }
                            }
                        } else {
                            load.Text(IndStringRaw(kGameStrList, kStatusDestroyed), bright);
                        }
                    } else {
                        if (shipClass[shipData[shipData[0].targetShip].type].shield <= 0)
                            load.Text(dstRect.left + 5, dstRect.bottom - 6, shieldString[3], bright);
                        else
                            load.Text(dstRect.left + 5, dstRect.bottom - 6, shieldString[4], bright);
                    }
                }
            } else {
                load.Text(dstRect.left + 5, dstRect.bottom - 6, shieldString[2], dim);
                load.Text(" ", dim);
                capacity = shipData[shipData[0].targetShip].ShieldCapacity();
                if (shipData[shipData[0].targetShip].shield <= capacity) {
                    load.Text(NumText((shipData[shipData[0].targetShip].shield / capacity * 100.0)), bright);
                    load.Text("%", bright);
                } else {
                    load.Text("100%", bright);
                }
            }

            if (shipData[shipData[0].targetShip].govt >= 0) {
                load.Text(dstRect.left, dstRect.right - 7, dstRect.bottom - 6,
                          govtTargStrings[shipData[shipData[0].targetShip].govt], dim, UI::Align::Right);
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
                    load.Text(dstRect.left, dstRect.right - 7, dstRect.bottom - 6, label, dim, UI::Align::Right);
                }
            }
            }
        }
    } else {
        load.Text(dstRect.left, dstRect.right, dstRect.top + 47, IndString(kGameStrList, kStatusNoTarget), dim,
                  UI::Align::Center);
    }

    if (preventStatusCopy == 0)
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, dstRect);
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
    UI::Painter load(UI::Surface::Load());
    const UI::TextSpec dim = { UI::Style::Status, *statusDimColor };
    const UI::TextSpec bright = { UI::Style::Status, *statusBrightColor };

    srcRect = statusOffRect[kWeapRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    load.Copy(StatusPicture(), srcRect, dstRect);
    if (shipData[0].selectedWeapon == -1) {
        load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, IndString(kGameStrList, kStatusNoWeapon), dim,
                  UI::Align::Center);
    } else {
        if (weaponStats[shipData[0].selectedWeapon].ammoType == -1 ||
            (weaponStats[shipData[0].selectedWeapon].flags2 & kWeapNoAmmoDisplay) != 0) {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, weapName[shipData[0].selectedWeapon], bright,
                      UI::Align::Center);
        } else if (weaponStats[shipData[0].selectedWeapon].ammoType > -kWeapAmmoFuelBase) {
            text = UpToNul(weapName[shipData[0].selectedWeapon]);
            text += " - ";
            if (weaponStats[shipData[0].selectedWeapon].ammoType >= 0 &&
                weaponStats[shipData[0].selectedWeapon].ammoType < 256 &&
                weaponStats[shipData[0].selectedWeapon].guidance != kGuidanceBay)
                text += NumText(shipData[0].weap[weaponStats[shipData[0].selectedWeapon].ammoType].ammo);
            else
                text += NumText(shipData[0].weap[shipData[0].selectedWeapon].ammo);
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, text, bright, UI::Align::Center);
        } else if (weaponStats[shipData[0].selectedWeapon].ammoType <= -kWeapAmmoFuelBase) {
            load.Text(dstRect.left, dstRect.right, dstRect.top + 0xc, weapName[shipData[0].selectedWeapon], bright,
                      UI::Align::Center);
        }
    }
    if (preventStatusCopy == 0) {
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, dstRect);
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
    UI::Painter load(UI::Surface::Load());
    const UI::TextSpec dim = { UI::Style::Status, *statusDimColor };
    const UI::TextSpec bright = { UI::Style::Status, *statusBrightColor };

    srcRect = statusOffRect[kCargoRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    load.Copy(StatusPicture(), srcRect, dstRect);
    for (i = 0; i <= 5; i++) {
        if (shipData[0].cargo[i] > 0) {
            load.Text(dstRect.left + 3, dstRect.top + (i + 1) * 14 - 2, statusCargoName[i], dim);
            load.Text(dstRect.left + 0x29, dstRect.top + (i + 1) * 14 - 2, NumText(shipData[0].cargo[i]), bright);
        }
    }
    load.Text(dstRect.left + 0x4d, dstRect.top + 0xc, IndStringRaw(kGameStrList, 0x13), dim);
    missionCargoCount = TotalFleetHolds() - TotalCargo(shipData);
    if (missionCargoCount < 0)
        missionCargoCount = 0;
    load.Text(dstRect.left + 0x6e, dstRect.top + 0xc, NumText(missionCargoCount), bright);
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
        load.Text(dstRect.left + 0x4d, dstRect.top + 0x1e, IndStringRaw(kGameStrList, 0x14), dim);
        if (missionCargoCount == 1) {
            load.Text(dstRect.left + 0x57, dstRect.top + 0x2e, TableEntry(cargoAbbrev, missionCargoType), bright);
        } else {
            if (junkCount == 1) {
                load.Text(dstRect.left + 0x57, dstRect.top + 0x2e,
                          FromField(junkData[junkIndex].abbrev, sizeof junkData[junkIndex].abbrev), bright);
            } else {
                load.Text(dstRect.left + 0x57, dstRect.top + 0x2e, IndStringRaw(kGameStrList, 0x15), bright);
            }
        }
    }
    text = creditsString;
    if (!text.empty())
        text[0] = (char)toupper((unsigned char)text[0]);
    load.Text(dstRect.left + 0x4d, dstRect.top + 0x42, text, dim);
    load.Text(":", dim);
    load.Text(dstRect.left + 0x57, dstRect.top + 0x52, CommaNumber(shipData[0].credits), bright);
    if (preventStatusCopy == 0) {
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, dstRect);
    }
}
