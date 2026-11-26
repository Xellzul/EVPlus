/* maininfo.cpp -- the main-screen object (tu-map O23, win 0x004873b0..0x00489d70). Notes: Research/notes/maininfo.md */

#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "sound.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "ships.h"
#include "viewscale.h"
#include "screen.h"
#include "uiscale.h"
#include "space.h"
#include "gamepaths.h"
#include "text.h"
#include "painter.h"

// FUNCTION: EVNOVA 0x004873b0
// MAC: _MainScreenUpdate i386 0x00017c0d ppc 0x0001b8e8
void MainScreenUpdate(short which)
{
    UI::MenuScale menu;
    Rect box;
    Boolean shown[6];
    short i;

    if (mainScreenBackgroundDirty != 0) {
        SetupInterfaceBackground();
        MainScreenAnimDraw();
        mainScreenBackgroundDirty = 0;
    }
    {
        UI::Painter work{UI::Surface(View::UIWork())};
        const UI::TextSpec value = { UI::Style::Menu, menuTextColor[0] };
        const UI::TextSpec label = { UI::Style::Menu, menuTextColor[1] };
        const short left = mainMenuCenter.h - 0x96;
        const short right = mainMenuCenter.h + 0x96;
        const short middle = mainMenuCenter.v + 0x136;

        if (which == 2) {
            work.Copy(UI::Surface::Load(), rolloverDestRect, rolloverDestRect);
            for (i = 0; i < 3; i++) {
                if (slidePort[i].world != NULL && slideIndex[i] < slideFrameCount[i])
                    work.Copy(UI::Surface::Load(), slideDestRect[i], slideDestRect[i]);
            }
        } else {
            work.Copy(UI::Surface::Load(), View::UIWork()->frameRect, View::UIWork()->frameRect);
        }
        SetRect(&box, standardPicRect.left + 0x115, standardPicRect.top + 0x10a,
                standardPicRect.left + 0x171, standardPicRect.top + 0x1be);
        if (creatingPilotFile != 0) {
            work.Text(left, right, middle, IndString(kGameStrList, 0x113), label, UI::Align::Center);
        } else {
            if (gameInProgress) {
                if (shipData->IsDying() == 0) {
                    work.Text((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0xfa), IndStringRaw(kGameStrList, 0xfb),
                              label);
                    work.Text((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x106), FromPascal(playerName), value);
                    work.Text((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0x11e), IndStringRaw(kGameStrList, 0xff),
                              label);
                    work.Text((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x12a), FromPascal(shipName), value);
                    work.Text((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0x142), IndStringRaw(kGameStrList, 0x100),
                              label);
                    work.Text((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x14e), ShipClassName(shipData->type),
                              value);
                    work.Text((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x15a),
                              FromNameField(shipClass[shipData->type].subtitle), value);
                    work.Text((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0xfa), IndStringRaw(kGameStrList, 0x116),
                              label);
                    work.Text((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x106), IndStringRaw(kGameStrList, 0x117),
                              label);
                    work.Text((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x112),
                              SystemIsInhabited(shipData->curSystem) ? LegalStatusText(shipData->curSystem)
                                                                     : IndStringRaw(kGameStrList, 0x18c),
                              value);
                    work.Text((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0x12a), IndStringRaw(kGameStrList, 0xfe),
                              label);
                    work.Text((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x136), CombatRatingText(), value);
                    work.Text((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0x14e), IndStringRaw(kGameStrList, 0xfc),
                              label);
                    work.Text((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x15a), DateMedText(gameTime), value);
                    i = shipClass[shipData->type].targetPictID;
                    if (targetInfoPic[i] != NULL) {
                        QDGetPictureBounds((PicHandle)targetInfoPic[i], &box);
                        OffsetRect(&box, -box.left, -box.top);
                        OffsetRect(&box,
                                   mainMenuCenter.h - (box.right - box.left) / 2,
                                   mainMenuCenter.v + 0x118);
                        UI::Painter(UI::Surface(View::UIBack())).Picture((PicHandle)targetInfoPic[i], box);
                        work.Copy(UI::Surface(View::UIBack()), box, box, UI::Blit::AddOver);
                    }
                } else {
                    if (playerName[1] == 'K' && playerName[2] == 'e' &&
                        playerName[3] == 'n' && playerName[4] == 'n' &&
                        playerName[5] == 'y') {
                        work.Text(left, right, middle, "Oh my God! They killed Kenny!", label, UI::Align::Center);
                    } else {
                        work.Text(left, right, middle, FromNameField(playerName) + " " + IndString(kGameStrList, 0x115),
                                  label, UI::Align::Center);
                    }
                }
            } else if (slideIndex[2] >= slideFrameCount[2]) {
                work.Text(left, right, middle, IndString(kGameStrList, 0x114), label, UI::Align::Center);
            }
        }

        if (slideIndex[0] >= slideFrameCount[0])
            shown[0] = shown[3] = 1;
        else
            shown[0] = shown[3] = 0;
        if (slideIndex[1] >= slideFrameCount[1])
            shown[1] = shown[4] = 1;
        else
            shown[1] = shown[4] = 0;
        if (slideIndex[2] >= slideFrameCount[2])
            shown[2] = shown[5] = 1;
        else
            shown[2] = shown[5] = 0;

        for (i = 0; i < 6; i++) {
            if (menuButtonSprite[i] != NULL && shown[i] != 0) {
                box = menuButtonSprite[i]->frameArray[0]->frameRect;
                OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                           menuTopCorner.v + menuButtonPos[i].v);
                SWSetSpriteLocation(menuButtonSprite[i],
                             (menuTopCorner.h + menuButtonPos[i].h),
                             (menuTopCorner.v + menuButtonPos[i].v));
                work.Sprite(BlitPixieRLEDrawProc, menuButtonSprite[i]->frameArray[0], box, box);
            }
        }
        for (i = 0; i < 6; i++) {
            if (menuButtonSprite[i] != NULL && shown[i] != 0) {
                box = menuButtonSprite[i]->frameArray[0]->frameRect;
                OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                           menuTopCorner.v + menuButtonPos[i].v);
                SWSetSpriteLocation(menuButtonSprite[i],
                             (menuTopCorner.h + menuButtonPos[i].h),
                             (menuTopCorner.v + menuButtonPos[i].v));
                work.Sprite(BlitPixieRLEDrawProc, menuButtonSprite[i]->frameArray[0], box, box);
            }
        }
    }
    MainScreenRolloverDraw();
    if (which == 1)
        ShowMainScreen();
}

/* the main screen as composed: the work page, and the load page's strip at the window's right edge */
void ShowMainScreen(void)
{
    UI::MenuScale menu;
    UI::Painter screen(UI::Surface::Screen());
    Rect box;
    Rect dst;

    screen.Copy(UI::Surface(View::UIWork()), View::UIWork()->frameRect, View::UIWork()->frameRect);
    box = loadPort.bounds;
    box.left = (box.right - kStatusBarWidth);
    dst = *GetCWindowPortRect((WindowRef)gWind);
    dst.left = (dst.right - kStatusBarWidth);
    screen.Copy(UI::Surface::Load(), box, dst);
}

#define kHotStripHeight 0x14
#define kBarHeight      0x1e

// FUNCTION: EVNOVA 0x00488080
// MAC: _MainScreenIdle i386 0x00018ae8 ppc 0x0001c6f0
void MainScreenIdle(void)
{
    UI::MenuScale menu;
    Rect strip;
    Point mouse;

    HandleIntroMusic(0);
    MainScreenAnimIdle();
    strip = *GetCWindowPortRect((WindowRef)gWind);
    strip.bottom = strip.top + kHotStripHeight;
    GetGlobalMouse(&mouse);
    if (PtInRectPoint(mouse, &strip)) {
        if (mBarHidden != 0) {
            InvalWindowPortRect((WindowRef)gWind);
            mBarHidden = 0;
        }
    } else if (mBarHidden == 0 && inBackground == 0 &&
               settings.windowed == 0) {
        InvalWindowPortRect((WindowRef)gWind);
        strip = *GetCWindowPortRect((WindowRef)gWind);
        strip.bottom = strip.top + kBarHeight;
        UI::Painter(UI::Surface::Screen()).Fill(strip, UI::kBlack);
        mBarHidden = 1;
    }
}

#define kVersionText        "Escape Velocity Plus vDEV 1"
#define kVersionGrey        0x8000
#define kVersionLeft        5
#define kVersionRaise       6           /* baseline above the backdrop's bottom edge */

/* Grey, in the lower-left corner of the main menu's backdrop. */
static void DrawVersion(void)
{
    const RGBColor grey = { kVersionGrey, kVersionGrey, kVersionGrey };

    UI::Painter(UI::Surface::Load()).Text(kVersionLeft, loadPort.bounds.bottom - kVersionRaise, kVersionText,
                                          { UI::Style::Version, grey });
}

/* mainMenuCenter is h-then-v, NOT QuickDraw's Point order */

#define kInterfacePictID    0x1f40
#define kMenuHalfWidth      512
#define kMenuHalfHeight     384


// FUNCTION: EVNOVA 0x00488380
// MAC: _SetupInterfaceBackground i386 0x0001631d ppc 0x0001a0b4
void SetupInterfaceBackground(void)
{
    UI::MenuScale menu;
    void *pic;

    pic = GetPicture(kInterfacePictID);
    if (pic != 0) {
        interfacePicRect = MenuArt()->bounds;
        CenterPict(pic, &interfacePicRect);
        mainMenuCenter.h = (interfacePicRect.left + interfacePicRect.right) / 2;
        mainMenuCenter.v = (interfacePicRect.top + interfacePicRect.bottom) / 2;
        menuTopCorner.h = mainMenuCenter.h - kMenuHalfWidth;
        menuTopCorner.v = mainMenuCenter.v - kMenuHalfHeight;
        {
            UI::Painter art{UI::Surface(*MenuArt())};

            art.Fill(MenuArt()->bounds, UI::kBlack);
            art.Picture((PicHandle)pic, interfacePicRect);
        }
        ShowMenuArt(MenuArt()->bounds);
        DrawVersion();
        QDReleaseResource((PicHandle)pic);
    }
    PlaceMainScreenAnim();
}


/* entergame -- EnterGameFromMainScreen and DoNewPilot. Notes: Research/notes/entergame.md */

// FUNCTION: EVNOVA 0x00489210
// MAC: _EnterGameFromMainScreen i386 0x00019ca7 ppc 0x0001d988
void EnterGameFromMainScreen(void)
{
    Rect box;

    crashlog("---");
    crashlog("entering game");
    playerIsLanded = 0;
    crashlog("adjusting sounds");
    AdjustSoundVolume();
    DisposeMainScreenSounds();
    crashlog("showing intro screen");
    if (!seenIntroScreen) {
        ShowIntroScreen();
        FadeScreenOut();
        seenIntroScreen = 1;
    } else {
        FadeScreenOut();
    }
    StopIntroMusic(1);
    crashlog("drawing stuff");
    LoadIntfResource();
    RandomizeStars();
    PropagateMissionBitEffects();
    if (systemData[shipData->curSystem].murk < 0)
        ClearStarfield();
    DrawMapInFloatingFrame();
    crashlog("drawing more stuff");
    ResetPlayerPrecalcedValues();

    HandleStellarSystemVisibility();
    HandleStellarSprites();
    PrepareAllSprites();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    ignoreKeys = 0;
    gameFrozen = 0;
    RedrawStatus(1);
    RefreshSAT();
    SWUpdateSpriteWorld(gSpriteWorld);
    Screen::ComposeView();
    statusRedraw[kRedrawCargo] = 1;
    escortMenuOnscreen = 0;
    mBarHidden = 1;
    gPause = 0;
    AdjustSoundVolume();
    fpsCounter = 0;
    lastFPSUpdateTime = TickCount();
    DisposeMainScreenAnim();
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    HideTheCursor();
    GetGlobalMouse(&lastMousePos);
    lastMouseMoveTime = TickCount();
    crashlog("changing window attributes");
    IterateInGameEventLoop(-1);
    RefreshWindowBuffer();
    crashlog("playing game");
    if (!EVBugResourceFieldTest(10))
        PlayGame();
    Screen::DropView();
    crashlog("paused");
    crashlog("resetting window attributes");
    if (strictPlay && shipData->IsDying() && shipData->type != kShipTypeEscapePod) {
        crashlog("deleting pilot file due to strict play death");
        DeletePilotFile(playerName);
    }
    crashlog("restoring stuff");
    KillVoiceSound();
    {
        UI::Painter screen(UI::Surface::Screen());

        screen.Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
        SetRect(&box, 0, 0, GetCWindowPortRect((WindowRef)gWind)->right, 20);
        screen.Fill(box, UI::kBlack);
    }
    InvalWindowPortRect((WindowRef)gWind);
    if (!gQuit) {
        SetupInterfaceBackground();
        LoadMainScreenAnim();
        while (StillDown())
            BusyIdle();
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
    }
    ShowTheCursor();
    UI::Painter(UI::Surface::Screen()).Fill(*GetCWindowPortRect((WindowRef)gWind), UI::kBlack);
    if (!gQuit) {
        crashlog("showing main screen again");
        InvalWindowPortRect((WindowRef)gWind);
        MainScreenAppearStart();
        MainScreenAppear();
        SetupInterfaceBackground();
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    crashlog("---");
}

#define kNewNameStrList  128

// FUNCTION: EVNOVA 0x00489d70
// MAC: _DoNewPilot i386 0x00018b0a ppc 0x0001c720
void DoNewPilot(void)
{
    uint32_t resType;
    Boolean wantStrictPlay;
    Str255 newShipName;
    Str255 charName;
    Str255 prompt;
    char **h;
    unsigned char *shipNameP;
    short i;
    short j;
    short sys;
    short k;
    Boolean ok;

    InvalWindowPortRect((WindowRef)gWind);
    wantStrictPlay = 0;
    GetIndString(newPlayerName[0], kNewNameStrList, Rand(3) + 1);
    GetIndString(newPlayerName[1], kNewNameStrList, Rand(3) + 4);
    shipNameP = newPlayerName[1];
    ok = DoNewPilotNameDialog(&wantStrictPlay);
    if (!ok)
        return;
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    CullNameString(newPlayerName[0]);
    CullNameString(shipNameP);
    TrimResourceName(newPlayerName[0]);
    TrimResourceName(shipNameP);
    if (DoesPilotExist(PilotFilePath(newPlayerName[0]).c_str())) {
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        GetIndString(prompt, kAlertStrList, 9);
        ok = EVYesNoDialog(prompt);
        if (!ok)
            return;
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    if (newPilotPopupChoice[0][0] == 0) {
        h = GetIndResource(kCharType, 1);
        if (h != 0) {
            GetResInfo(h, &i, &resType, newPilotPopupChoice[0]);
            ReleaseResource(h);
        }
    }
    i = GetCharResourceShipType(newPilotPopupChoice[0]);
    ToPascal(IndString(kGameStrList, 121) + " " + TableEntry(shipyardLongNames, i) + ": ", prompt);
    GetIndString(charName, kNewNameStrList, Rand(3) + 7);
    ok = EVTextInputDialog(prompt, charName, 64);
    if (!ok)
        return;
    creatingPilotFile = 1;
    if (tolower(newPilotPopupChoice[1][1]) == 'm')
        playerGender = 1;
    else
        playerGender = 0;
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    onPlanet = 0;
    ResetPlayer(1);
    ResetPlayerRecord(1);
    ResetGameData();
    for (i = 0; i < kMaxOutfits; i++)
        shipSystems[i] = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }
    ActivateCharResource(newPilotPopupChoice[0], 1);
    for (i = 0; i < kMaxSystems; i++)
        systemData[i].exploreLevel = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = shipClass[shipData->type].weapCount[i];
        if (shipClass[shipData->type].weapAmmo[i] > 0) {
            if (weaponStats[i].ammoType >= 0 && weaponStats[i].ammoType < kMaxWeapTypes)
                shipData->weap[weaponStats[i].ammoType].ammo =
                    shipClass[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = shipClass[shipData->type].weapAmmo[i];
        }
    }
    ShipStatsToSystemInfo();
    for (k = 0; k < 8; k++) {
        if (shipClass[shipData->type].defOutfCount[k] > 0)
            shipSystems[shipClass[shipData->type].defOutfID[k]] +=
                shipClass[shipData->type].defOutfCount[k];
    }
    for (i = 0; i < kMaxSystemLinks; i++) {
        if (systemData[shipData->curSystem].links[i] != -1
            && systemData[systemData[shipData->curSystem].links[i]].loaded != 0) {
            sys = FindFirstCoLocatedSystem(systemData[shipData->curSystem].links[i]);
            if (sys >= 0 && sys < kMaxSystems)
                systemData[sys].exploreLevel = 1;
        }
    }
    sys = FindFirstCoLocatedSystem(shipData->curSystem);
    if (sys >= 0 && sys < kMaxSystems)
        systemData[sys].exploreLevel = 1;
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    systemData[shipData->curSystem].reinforceTimer = -1.0f;
    systemData[shipData->curSystem].reinforceDaysLeft = 0;
    shipData->shield = shipData->ShieldCapacity();
    shipData->armor = shipData->ArmorCapacity();
    shipData->fuel = shipData->FuelCapacity();
    LoadObjectData();
    DeleteAllComputerShips(1, 1);
    ExploreNebulaeFromSystem(shipData->curSystem);
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    missionSoundID = -1;
    killShots = killBoxes = killExplods = killSmoke = 1;
    pstrcpy(newShipName, textInputString);
    CullNameString(newShipName);
    pstrcpy(shipName, newShipName);
    pstrcpy(playerName, newPlayerName[0]);
    pstrcpy(playerNickname, shipNameP);
    gameTime = scenarioStartDate;
    gameInProgress = 1;
    ApplyPilotGameSize();
    strictPlay = wantStrictPlay;
    shipData->deathTimer = -1.0f;
    shipData->ionization = 0.0f;
    shipData->ionColor = 0;
    shipData->frameTimer = 0.0f;
    shipData->bankDir = 0;
    shipData->escapePods = -1;
    newbieFlag = -3;
    PropagateMissionBitEffects();
    sys = FindNearestStellar(shipData);
    if (sys == -1) {
        for (k = 0; k < kNavSlots; k++) {
            if (systemData[shipData->curSystem].navDefault[k] != -1) {
                sys = systemData[shipData->curSystem].navDefault[k];
                break;
            }
        }
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & kSpobStartsDestroyed) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays >= 0)
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
            else
                stellarData[i].deadDaysLeft = 1;
        } else {
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadDaysLeft = 0;
        }
    }
    ActivateCharResource(newPilotPopupChoice[0], 0);
    if (sys != -1)
        SavePilotFile(sys);
    else
        SavePilotFile(0);
    SetupShipsInSystem(shipData->curSystem, 1);
    HandleStellarSystemVisibility();
    CreateAsteroids();
    GetCharResourceIntroIDs(newPilotPopupChoice[0], &introTextID, introPictID, introPictDelay);
    SWHideAllSprites(gSpriteWorld);
    creatingPilotFile = 0;
    SetupInterfaceBackground();
    MainScreenAnimDraw();
    MainScreenUpdate(1);
}
