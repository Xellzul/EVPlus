/* statusradar.cpp -- DrawStatusRadar (win link order 0x0045d600), the fifth member of the status-painter run and the one function of it that is NOT in statusparts.cpp. Notes: Research/notes/statusradar.md */

#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "stellar.h"
#include "space.h"
#include "game.h"
#include "painter.h"

/* the radar is statusOffRect[0] */
#define kRadarRect          0
#define kRadarScale         0.03125     /* 1/32: system units to radar pixels */
#define kRadarStaticFrames  10
#define kRadarArrowDist     7000000.0   /* draw the departure arrow past this */

// FUNCTION: EVNOVA 0x0045d600
// MAC: _DrawStatusRadar i386 0x000496e0 ppc 0x000520ec
void DrawStatusRadar(Boolean highlight)
{
    Rect dstRect;
    Rect srcRect;
    Rect blipRect;
    FloatPoint tip;
    FloatPoint barb;
    RGBColor color;
    RGBColor ink;
    int32_t cx;
    int32_t cy;
    Boolean hasIFF;
    Boolean hasDensity;
    Boolean visible;
    Boolean isStatic;
    int32_t i;
    int32_t stellar;
    int32_t size;
    int32_t angle;
    int32_t links;

    if (statusOffRect[kRadarRect].left >= statusOffRect[kRadarRect].right ||
        statusOffRect[kRadarRect].top >= statusOffRect[kRadarRect].bottom)
        return;
    if (playerIsLanded)
        highlight = 1;
    hasIFF = shipData->HasIFF();
    hasDensity = shipData->HasDensityScanner();
    UI::Painter load(UI::Surface::Load());

    srcRect = statusOffRect[kRadarRect];
    dstRect = srcRect;
    OffsetRect(&dstRect, GetCWindowPortRect(gWind)->right - kStatusBarWidth, 0);
    isStatic = highlight == 0 && interferenceThisFrame != 0;
    if (isStatic) {
        cx = Rand(kRadarStaticFrames);
        if (staticPat[cx] != NULL)
            load.Pattern(dstRect, (PixPatHandle)staticPat[cx]);
    } else {
        if (hasIFF != 0)
            load.Fill(dstRect, UI::kBlack);
        else
            load.Copy(UI::Surface(statusOffPort), srcRect, dstRect);
        cx = (dstRect.left + dstRect.right) * 0.5;
        cy = (dstRect.top + dstRect.bottom) * 0.5;
        if (highlight == 0) {
            for (i = 0; i < kNavSlots; i++) {
                stellar = systemData[shipData[0].curSystem].navDefault[i];
                if (stellar != -1) {
                    blipRect.left = blipRect.right =
                        (cx + (int32_t)(stellarData[stellar].pos.h -
                                     shipData[0].pos.x) * kRadarScale);
                    blipRect.top = blipRect.bottom =
                        (cy + (int32_t)(stellarData[stellar].pos.v -
                                     shipData[0].pos.y) * kRadarScale);
                    if ((stellarData[stellar].flags & kSpobIsStation) != 0 ||
                        (stellarData[stellar].flags2 & kSpobGateOrWormhole) != 0) {
                        if (hasIFF != 0) {
                            ColorCodeStellar(&stellarData[stellar], &color);
                            ink = color;
                        } else {
                            ink = *radarStatusColor[0];
                        }
                        InsetRect(&blipRect, -1, -1);
                        SectRect(&blipRect, &dstRect, &blipRect);
                        if (blipRect.left != 0 || blipRect.top != 0 ||
                            blipRect.left != 0 || blipRect.right != 0)
                            load.Frame(blipRect, ink);
                    } else {
                        if (hasIFF != 0) {
                            ColorCodeStellar(&stellarData[stellar], &color);
                            ink = color;
                        } else {
                            ink = *radarStatusColor[0];
                        }
                        size = 0x20;
                        if (masterStellarSprite[stellarData[stellar].type] != NULL &&
                            masterStellarSprite[stellarData[stellar].type]->frameArray != NULL)
                            size = GetSpriteXSize(masterStellarSprite[stellarData[stellar].type]);
                        if (size >= 200)
                            InsetRect(&blipRect, -3, -3);
                        else if (size >= 90)
                            InsetRect(&blipRect, -2, -2);
                        else
                            InsetRect(&blipRect, -1, -1);
                        SectRect(&blipRect, &dstRect, &blipRect);
                        if (blipRect.left != 0 || blipRect.top != 0 ||
                            blipRect.left != 0 || blipRect.right != 0)
                            load.Oval(blipRect, ink);
                    }
                }
            }
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active != 0 &&
                    shipData[i].curSystem == shipData[0].curSystem) {
                    if (shipData[i].IsCloaked() != 0) {
                        if (shipData[i].CloakVisibleOnRadar() != 0 ||
                            shipData[0].seesCloaked[1] == 1)
                            visible = 1;
                        else
                            visible = 0;
                    } else
                        visible = 1;
                    if (visible) {
                        blipRect.left = blipRect.right =
                            (cx + (int32_t)(shipData[i].pos.x -
                                         shipData[0].pos.x) * kRadarScale);
                        blipRect.top = blipRect.bottom =
                            (cy + (int32_t)(shipData[i].pos.y -
                                         shipData[0].pos.y) * kRadarScale);
                        if (shipData[0].targetShip == i) {
                            if (radarCount != 0) {
                                if (hasIFF != 0) {
                                    ink = rgbLightGrey;
                                } else {
                                    ink = *radarStatusColor[1];
                                }
                            } else if (hasIFF != 0) {
                                ColorCodeShip(&shipData[i], &color);
                                ink = color;
                            } else {
                                ink = *radarStatusColor[0];
                            }
                        } else if (hasIFF != 0) {
                            ColorCodeShip(&shipData[i], &color);
                            ink = color;
                        } else {
                            ink = *radarStatusColor[0];
                        }
                        if (hasDensity != 0 &&
                            shipClass[shipData[i].type].mass >= 100) {
                            InsetRect(&blipRect, -1, -1);
                            SectRect(&blipRect, &dstRect, &blipRect);
                            if (blipRect.left != 0 || blipRect.top != 0 ||
                                blipRect.left != 0 || blipRect.right != 0)
                                load.Frame(blipRect, ink);
                        } else {
                            if (blipRect.left >= dstRect.left &&
                                blipRect.left <= dstRect.right &&
                                blipRect.top >= dstRect.top &&
                                blipRect.top <= dstRect.bottom) {
                                load.Line(blipRect.left, blipRect.top, blipRect.left, blipRect.top, ink);
                            }
                        }
                    }
                }
            }
        }
    }
    // BUG (all versions): static leaves cy unset, cx the pattern index; dot and arrow skipped (exe draws them off the radar)
    if (!isStatic) {
        if (hasIFF != 0) {
            ColorCodeShip(shipData, &color);
            ink = color;
        } else {
            ink = *radarStatusColor[1];
        }
        load.Line(cx, cy, cx, cy, ink);
        if (radarCount != 0 && shipData[0].navTargetKind != 3) {
            links = 0;
            for (i = 0; i < kNavSlots; i++) {
                if (systemData[shipData[0].curSystem].navDefault[i] != -1)
                    links++;
            }
            if (links > 0) {
                tip.x = tip.y = 0.0f;
                if (XYFloatSquaredDist(&shipData[0].pos, &tip) > kRadarArrowDist) {
                    angle = CalcInterceptAngle(&shipData[0].pos, &tip);
                    tip.x = barb.x = cx;
                    tip.y = barb.y = cy;
                    Accel(angle, 25.0f, &tip);
                    Accel(angle, 50.0f, &barb);
                    ink = *radarStatusColor[0];
                    load.Line(tip.x, tip.y, barb.x, barb.y, ink);
                    tip.x = barb.x;
                    tip.y = barb.y;
                    Accel((angle + 0x87), 6.0f, &tip);
                    load.Line(barb.x, barb.y, tip.x, tip.y, ink);
                    tip.x = barb.x;
                    tip.y = barb.y;
                    Accel((angle - 0x87), 6.0f, &tip);
                    load.Line(barb.x, barb.y, tip.x, tip.y, ink);
                }
            }
        }
    }
    if (preventStatusCopy == 0)
        UI::Painter(UI::Surface::Screen()).Copy(UI::Surface::Load(), dstRect, dstRect);
}
