/* dosystemmap.cpp -- tu-map object O25: the galaxy map dialog, its drawing, filters and jump routes. Notes: Research/notes/dosystemmap.md */

#include "prototypes.h"
#include "ships.h"
#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "sound.h"
#include "stellar.h"
#include "gamedialog.h"
#include "viewscale.h"
#include "space.h"
#include "buttons.h"
#include "text.h"
#include "painter.h"

/* the mac plots the map arrows as colour icons */

/* mapZoom and floatingMapZoom: one .data run, in address order. */
double mapZoom = 0.5625;                         /* 0x005759d8 (mac name) */
double floatingMapZoom = 0.5625;                 /* 0x005759e0 (see mapZoom above) */

#define kMapDlogID     2000
#define kMapPictID     0x213d
#define kNebulaPictBase 0x251c

/* DLOG 2000's items; 1 is kItemAccept and 3 kMapPictItem */
#define kMapInfoItem        2   /* the selected system's panel */
#define kMapZoomOutItem     4
#define kMapZoomInItem      5
#define kMapRouteItem       6   /* the destination and route panel */
#define kMapCursorItem      7
#define kMapClearRouteItem  8
#define kMapBordersItem     9
#define kMapSearchItem      10

#define kPriceLevelStrings  0xc9    /* STR# kGameStrList: Low, Med, High */
#define kDisasterStrings    0xcc    /* STR# kGameStrList: Higher, Lower */
#define kGoodsHeight        0x48    /* room for Goods Traded above Services: six lines */

static void DrawMap(UI::Painter &p, Rect box);
static void DrawMapInfluence(UI::Painter &p, Rect bounds);


namespace {

// FUNCTION: EVNOVA 0x004a3aa0
// MAC: _DoSystemMap i386 0x000121a3 ppc 0x00015bbc
/* The galaxy map: a click selects a system (in flight, targets it); a shift-click extends or cuts back the
   hyper route. */
class MapDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short itemHit) override;
    static Boolean Filter(DialogPtr dlg, const DialogEvent &event, short *itemHit);

    short routeLen;     /* the hyper route's length */
    Rect box;           /* the map picture item's rect */
};

void MapDialog::Run()
{
    short sys;
    short found;
    short i;
    short j;
    short savedStellar;
    short x;
    short y;
    short itemType;
    Handle item;
    unsigned char savedShow;
    Rect r;
    Rect box2;
    Rect *pr;
    Picture nebulae[kMaxNebulae][7];

    if (screenFaded != 0)
        return;
    ShowTheCursor();
    dialog.Open(kMapDlogID, MapDialogUpdate, Filter, &mapDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(mapBackgroundPic);
    savedShow = settings.showMapBorders;
    influenceCoordsCached = 0;
    savedStellar = shipData->targetStellar;
    if (mapZoom <= 0.0)
        mapZoom = 1.0;
    CompactHyperRoute();
    routeLen = 1;
    for (i = 1; i < kMaxHyperRoute; i++) {
        if (hyperRoute[i] == -1) {
            routeLen = i;
            break;
        }
    }
    showClearRouteButton = 0;
    if (hypergateMapMode == 0) {
        for (i = 1; i < kMaxHyperRoute; i++) {
            if (hyperRoute[i] != -1)
                showClearRouteButton = 1;
        }
    }
    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored != 0) {
            for (j = 0; j < 7; j++) {
                nebulae[i][j].Bind(nebulaPic[i][j]);
                nebulae[i][j].Load(kNebulaPictBase + i * 7 + j);
            }
        }
    }
    mapCursorSwitchRgn = 0;
    partialMapDialogUpdate = 0;
    y = 0;
    do {
        for (sys = 0; sys < kMapBordersWidth; sys++) {
            for (i = 0; i < 4; i++) {
                influenceOwner[sys][y][i] = 0;
                influenceValue[sys][y][i] = 0;
            }
        }
    } while (++y < kMapBordersHeight);
    RecalcMissionSystsForMap();
    if (shipData->navTargetKind == kNavTargetLink && shipData->targetStellar != -1 && onPlanet == 0)
        selSystem = FindActiveCoLocatedSystem(
            systemData[shipData->curSystem].links[shipData->targetStellar]);
    else
        selSystem = shipData->curSystem;
    backdrop.Load(kMapPictID);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(mapDialog);
    MapDialogUpdate();
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    if (settings.showMapBorders != 0)
        RecalcAllMapInfluence(&box);
    Loop();
    x = (GetDialogPortRect(mapDialog)->right - GetDialogPortRect(mapDialog)->left) / 2;
    y = (GetDialogPortRect(mapDialog)->bottom - GetDialogPortRect(mapDialog)->top) / 2;
    while (KeyCheck(keyPrefs.map))
        BusyIdle();
    dialog.Close();
    for (i = 0; i < kMaxNebulae; i++)
        for (j = 0; j < 7; j++)
            nebulae[i][j].Release();
    backdrop.Release();
    found = (GetCWindowPortRect(gWind)->right - GetCWindowPortRect(gWind)->left) / 2;
    sys = (GetCWindowPortRect(gWind)->bottom - GetCWindowPortRect(gWind)->top) / 2;
    SetRect(&r, found - x, sys - y, found + x, sys + y);
    pr = GetCWindowPortRect(gWind);
    box2 = *pr;
    box2.left = box2.right - kStatusBarWidth;
    if (SectRect(&r, &box2, &box2))
        RedrawStatus(1);
    RedrawDialogs(1);
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    if (portDialog != NULL && playerIsLanded != 0) {
        shipData->targetStellar = savedStellar;
    } else {
        if (hypergateMapMode == 0)
            CueNextHyperRouteDest();
    }
    if (portDialog == NULL && playerIsLanded == 0
        && settings.showMapBorders != savedShow)
        SavePrefs();
    FlushEvents(7, 0);
}

void MapDialog::Hit(short itemHit)
{
    short sys;
    short found;
    short i;
    short j;
    int32_t last;
    short prev;
    short s2;
    short s3;
    short prevV;
    short prevH;
    short sy;
    short w;
    short h;
    short itemType;
    Handle item;
    Boolean inRoute;
    Boolean selected;
    Boolean hyperlinked;
    Rect r;
    Point mouse;
    Rect box2;
    Point anchor;
    MapScroll savedScroll;

    if (itemHit == kItemAccept)
        done = 1;
    if (itemHit == kMapCursorItem) {
        cursorHidden = 0;
        ShowTheCursor();
    }
    if (itemHit == kMapBordersItem) {
        settings.showMapBorders = (settings.showMapBorders == 0);
        InvalDialogItem(mapDialog, kMapBordersItem);
        InvalDialogItem(mapDialog, kMapPictItem);
        if (settings.showMapBorders != 0)
            RecalcAllMapInfluence(&box);
    }
    if (itemHit == kMapZoomOutItem && canZoomOut != 0) {
        mapZoom = mapZoom * 1.333333;
        InvalDialogItem(mapDialog, kMapInfoItem);
        InvalDialogItem(mapDialog, kMapPictItem);
        InvalDialogItem(mapDialog, kMapZoomOutItem);
        InvalDialogItem(mapDialog, kMapZoomInItem);
        InvalDialogItem(mapDialog, kMapRouteItem);
        if (settings.showMapBorders != 0)
            RecalcAllMapInfluence(&box);
    }
    if (itemHit == kMapZoomInItem && canZoomIn != 0) {
        mapZoom = mapZoom * 0.75;
        InvalDialogItem(mapDialog, kMapInfoItem);
        InvalDialogItem(mapDialog, kMapPictItem);
        InvalDialogItem(mapDialog, kMapZoomOutItem);
        InvalDialogItem(mapDialog, kMapZoomInItem);
        InvalDialogItem(mapDialog, kMapRouteItem);
        if (settings.showMapBorders != 0)
            RecalcAllMapInfluence(&box);
    }
    if (itemHit == kMapClearRouteItem) {
        for (i = 1; i < kMaxHyperRoute; i++)
            hyperRoute[i] = -1;
        hyperRoute[0] = shipData->curSystem;
        routeLen = 1;
        showClearRouteButton = 0;
        InvalDialogItem(mapDialog, kMapPictItem);
        InvalDialogItem(mapDialog, kMapRouteItem);
        InvalDialogItem(mapDialog, kMapClearRouteItem);
        MapDialogUpdate();
        shipData->targetStellar = -1;
        statusRedraw[kRedrawNav] = 1;
        DrawStatus();
    }
    if (itemHit == kMapSearchItem)
        DoSystemSearch();
    if (itemHit == kMapPictItem) {
        GetMouse(&mouse);
        GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box2);
        w = box2.right - box2.left;
        h = box2.bottom - box2.top;
        found = -1;
        for (sys = 0; sys < kMaxSystems; sys++) {
            if (found != -1)
                break;
            found = -1;
            if (systemData[sys].loaded != 0) {
                sy = systemData[sys].pos.v;
                r.left = (box2.left + w / 2
                                 + systemData[sys].pos.h / mapZoom
                                 - mapScroll.h / mapZoom);
                r.top = (box2.top + h / 2
                                + sy / mapZoom
                                - mapScroll.v / mapZoom);
                r.right = r.left;
                r.bottom = r.top;
                InsetRect(&r, -10, -10);
                if (PtInRectPoint(mouse, &r)) {
                    inRoute = selected = hyperlinked = 0;
                    for (s3 = 0; s3 < 32; s3++) {
                        if (sys == missionSyst[s3] && missionSyst[s3] != -1) {
                            inRoute = 1;
                            break;
                        }
                    }
                    if (greenMissionSyst == sys)
                        selected = 1;
                    if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                        && hypergateMapModeStellar < kMaxStellars) {
                        for (i = 0; i < kSpobHyperLinks; i++) {
                            if (stellarData[hypergateMapModeStellar].hyperLinks[i] != -1) {
                                s3 = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[i]].system;
                                if (s3 < 0 || s3 >= kMaxSystems)
                                    s3 = FindSystemFromStellar(stellarData[hypergateMapModeStellar].hyperLinks[i]);
                                s3 = FindActiveCoLocatedSystem(s3);
                                if (s3 == sys) {
                                    hyperlinked = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (hypergateMapMode != 0 && hyperlinked == 0
                        && sys != shipData->curSystem)
                        continue;
                    if (systemData[sys].known != 0) {
                        found = sys;
                        break;
                    }
                    if (inRoute != 0 || selected != 0 || hyperlinked != 0) {
                        found = sys;
                        break;
                    }
                    if ((KeyCheck(kScanLShift) == 0
                         && KeyCheck(kScanRShift) == 0)
                        || routeLen <= 1) {
                        prev = shipData->curSystem;
                    } else {
                        prev = hyperRoute[routeLen - 1];
                        if (systemData[prev].pos.h == systemData[sys].pos.h
                            && systemData[prev].pos.v == systemData[sys].pos.v) {
                            found = sys;
                            break;
                        }
                    }
                    for (i = 0; i < kMaxSystemLinks; i++) {
                        if (systemData[prev].links[i] != -1) {
                            s2 = FindActiveCoLocatedSystem(systemData[prev].links[i]);
                            if (s2 == sys) {
                                found = s2;
                                break;
                            }
                            if (systemData[s2].pos.h == systemData[sys].pos.h
                                && systemData[s2].pos.v == systemData[sys].pos.v) {
                                found = s2;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (found != -1)
            found = FindActiveCoLocatedSystem(found);
        if (found != -1) {
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            if ((KeyCheck(kScanLShift) != 0
                 || KeyCheck(kScanRShift) != 0)
                && hypergateMapMode == 0) {
                CompactHyperRoute();
                routeLen = -1;
                if (FindFirstCoLocatedSystem(shipData->curSystem)
                    == FindFirstCoLocatedSystem(found)) {
                    for (i = 0; i < kMaxHyperRoute; i++)
                        hyperRoute[i] = -1;
                }
                for (i = 1; i < kMaxHyperRoute; i++) {
                    if (hyperRoute[i] == -1)
                        break;
                    if (FindFirstCoLocatedSystem(hyperRoute[i])
                        == FindFirstCoLocatedSystem(found)) {
                        for (j = i; j < kMaxHyperRoute; j++)
                            hyperRoute[j] = -1;
                        break;
                    }
                }
                for (i = 1; i < kMaxHyperRoute; i++) {
                    if (hyperRoute[i] == -1) {
                        routeLen = i;
                        break;
                    }
                }
                if (routeLen == -1) {
                    if (hyperRoute[0] == -1)
                        hyperRoute[0] = shipData->curSystem;
                    routeLen = 1;
                }
                inRoute = 0;
                if (routeLen > 1) {
                    prev = hyperRoute[routeLen - 1];
                    s2 = FindActiveCoLocatedSystem(prev);
                    if (s2 != prev && s2 != -1)
                        prev = s2;
                } else
                    prev = shipData->curSystem;
                if (systemData[prev].exploreLevel > 0
                    || systemData[found].known != 0) {
                    for (i = 0; i < kMaxSystemLinks; i++) {
                        if (systemData[prev].links[i] != -1) {
                            s2 = FindActiveCoLocatedSystem(systemData[prev].links[i]);
                            if (s2 == found) {
                                inRoute = 1;
                                break;
                            }
                        }
                    }
                }
                if (routeLen > 0) {
                    last = routeLen - 1;
                    s2 = hyperRoute[last];
                    if (s2 >= 0 && s2 < kMaxSystems) {
                        if (systemData[found].pos.h == systemData[s2].pos.h
                            && systemData[found].pos.v == systemData[s2].pos.v) {
                            hyperRoute[last] = -1;
                            routeLen--;
                            inRoute = 0;
                        }
                    }
                }
                if (inRoute != 0) {
                    if (routeLen != -1) {
                        if (hyperRoute[0] == -1)
                            hyperRoute[0] = shipData->curSystem;
                        hyperRoute[routeLen] = found;
                        routeLen++;
                        showClearRouteButton = 1;
                    } else {
                        hyperRoute[0] = shipData->curSystem;
                        hyperRoute[1] = found;
                        routeLen = 2;
                        showClearRouteButton = 1;
                    }
                    InvalDialogItem(mapDialog, kMapPictItem);
                    InvalDialogItem(mapDialog, kMapRouteItem);
                    InvalDialogItem(mapDialog, kMapClearRouteItem);
                    {
                        UI::Painter load(UI::Surface::Load());

                        load.Fill(*GetDialogPortRect(mapDialog), UI::kBlack);
                        if (mapBackgroundPic != NULL)
                            load.Picture(mapBackgroundPic, *GetDialogPortRect(mapDialog));
                    }
                    MapButtons().DrawOnDialog(-1);
                    selSystem = found;
                }
            } else {
                selSystem = found;
                if (onPlanet == 0) {
                    if (hypergateMapMode != 0) {
                        if (hyperlinked != 0) {
                            shipData->navTargetKind = kNavTargetGate;
                            shipData->targetStellar = selSystem;
                        } else {
                            shipData->navTargetKind = -1;
                            shipData->targetStellar = -1;
                        }
                    } else {
                        s2 = -1;
                        for (i = 0; i < kMaxSystemLinks; i++) {
                            if (systemData[shipData->curSystem].links[i] != -1) {
                                if (found == FindActiveCoLocatedSystem(systemData[shipData->curSystem].links[i])) {
                                    s2 = i;
                                    break;
                                }
                            }
                        }
                        if (s2 != -1) {
                            shipData->navTargetKind = kNavTargetLink;
                            shipData->targetStellar = s2;
                            statusRedraw[kRedrawNav] = 1;
                            DrawStatus();
                        } else {
                            shipData->targetStellar = -1;
                            statusRedraw[kRedrawNav] = 1;
                            DrawStatus();
                        }
                    }
                }
            }
        } else {
            if (StillDown()) {
                GetMouse(&anchor);
                savedScroll = mapScroll;
                while (StillDown()) {
                    BusyIdle();
                    prevV = mouse.v;
                    prevH = mouse.h;
                    GetMouse(&mouse);
                    mapScroll.h = (savedScroll.h
                                          - (mouse.h - anchor.h) * mapZoom);
                    mapScroll.v = (savedScroll.v
                                          - (mouse.v - anchor.v) * mapZoom);
                    ScrollMapDisplay(mouse.h - prevH, mouse.v - prevV);
                    nv_UpdateDialogWindow(0);
                }
                MouseFlushWheel();      /* no zoom from notches turned during the drag */
                if (settings.showMapBorders != 0)
                    RecalcAllMapInfluence(&box);
                if (mapScroll.h != savedScroll.h
                    || mapScroll.v != savedScroll.v)
                    MapDialogUpdate();
            }
        }
    }
}

static constexpr short kMapSlotItems[] = {
    kItemAccept, kMapZoomOutItem, kMapZoomInItem, kMapClearRouteItem, kMapBordersItem, kMapSearchItem,
};

// FUNCTION: EVNOVA 0x004a74f0
// MAC: _MapFilter i386 0x00013c63 ppc 0x000175a8
Boolean MapDialog::Filter(DialogPtr dlg, const DialogEvent &event, short *itemHit)
{
    Handle item;
    EventChar key;
    short i;
    short j;
    short k;
    short n;
    short count;
    short cur;
    short sys;
    Boolean ok[kMaxSystemLinks];
    short list[kSpobHyperLinks];
    Rect box;
    short itemType;
    DialogInput what;
    Point pt;

    if (mapZoom > 0.5)
        canZoomIn = 1;
    else
        canZoomIn = 0;
    if (mapZoom < 2.0)
        canZoomOut = 1;
    else
        canZoomOut = 0;
    didUpdateMap = 0;
    what = event.what;
    if (what == DialogInput::Key) {
        key = tolower(event.ch);
        if (IsReturnChar(key) || key == kEscapeChar) {
            *itemHit = kItemAccept;
            return 1;
        } else if ((key == kTabChar || key == '\\') && onPlanet == 0) {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxStellars) {
                n = 0;
                for (i = 0; i < kSpobHyperLinks; i++)
                    list[i] = -1;
                for (i = 0; i < kSpobHyperLinks; i++) {
                    sys = stellarData[hypergateMapModeStellar].hyperLinks[i];
                    // BUG (all versions): sys is a stellar index but is bounded by kMaxSystems, not kMaxStellars
                    if (sys >= 0 && sys < kMaxSystems) {
                        list[n] = FindSystemFromStellar(sys);
                        n++;
                    }
                }
                count = n;
                if (count > 0) {
                    cur = -1;
                    for (i = 0; i < count; i++) {
                        if (selSystem == list[i]) {
                            cur = i;
                            break;
                        }
                    }
                    if (cur == -1)
                        cur = 0;
                    else if ((event.mods & shiftKey) != 0) {
                        if (cur == 0)
                            cur = n - 1;
                        else
                            cur -= 1;
                    } else
                        cur = (cur + 1) % n;
                    shipData[0].navTargetKind = kNavTargetGate;
                    shipData[0].targetStellar = list[cur];
                    selSystem = list[cur];
                    InvalDialogItem(mapDialog, kMapInfoItem);
                    InvalDialogItem(mapDialog, kMapPictItem);
                    InvalDialogItem(mapDialog, kMapRouteItem);
                }
            } else {
                shipData[0].navTargetKind = kNavTargetLink;
                count = 0;
                for (i = 0; i < kMaxSystemLinks; i++) {
                    ok[i] = 0;
                    if (systemData[shipData[0].curSystem].links[i] != -1) {
                        if (FindActiveCoLocatedSystem(
                                systemData[shipData[0].curSystem].links[i]) != -1) {
                            ok[i] = 1;
                            count++;
                        }
                    }
                }
                if (count > 0) {
                    do {
                        shipData[0].targetStellar += 1;
                        if (shipData[0].targetStellar >= kMaxSystemLinks)
                            shipData[0].targetStellar = 0;
                        if (shipData[0].targetStellar < 0)
                            shipData[0].targetStellar = kMaxSystemLinks - 1;
                    } while (ok[shipData[0].targetStellar] == 0);
                } else
                    shipData[0].targetStellar = -1;
                statusRedraw[kRedrawNav] = 1;
                InvalDialogItem(mapDialog, kMapInfoItem);
                InvalDialogItem(mapDialog, kMapPictItem);
                InvalDialogItem(mapDialog, kMapRouteItem);
                DrawStatus();
                if (shipData[0].navTargetKind == kNavTargetLink && shipData[0].targetStellar != -1)
                    selSystem = FindActiveCoLocatedSystem(
                        systemData[shipData[0].curSystem]
                            .links[shipData[0].targetStellar]);
                else
                    selSystem = shipData[0].curSystem;
            }
        } else if (key == 'c') {
            *itemHit = kMapClearRouteItem;
            return 1;
        } else if (key == 'b') {
            *itemHit = kMapBordersItem;
            return 1;
        } else if (key == ' ') {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;
            selSystem = shipData[0].curSystem;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == '-') {
            *itemHit = kMapZoomOutItem;
            return 1;
        } else if (key == '+' || key == '=') {
            *itemHit = kMapZoomInItem;
            return 1;
        } else if (key == kLeftArrowChar) {
            mapScroll.h -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kRightArrowChar) {
            mapScroll.h += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kUpArrowChar) {
            mapScroll.v -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kDownArrowChar) {
            mapScroll.v += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == 'f' || key == 's') {
            *itemHit = kMapSearchItem;
            return 1;
        } else if (KeyCheck(keyPrefs.map) != 0) {
            *itemHit = kItemAccept;
            return 1;
        }
    }
    if (event.what == DialogInput::Click) {
        pt = event.where;
        GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
        if (PtInRect(pt.h, pt.v, &box) != 0) {
            *itemHit = kMapPictItem;
            return 1;
        }
        MapDialogUpdate();
        return ButtonItemHit(MapButtons().Track(pt), kMapSlotItems, itemHit);
    }
    if (event.what == DialogInput::Idle && settings.showMapBorders != 0
        && influenceCoordsCached != 0) {
        if (nextInfluenceSyst < 0) {
            nextInfluenceSyst = 0;
        } else if (nextInfluenceSyst <= kMaxSystems) {
            influenceStepSize = 1;
            for (i = 0; i < kMapBordersWidth; i++) {
                for (j = 0; j < kMapBordersHeight; j++) {
                    for (k = 0; k < 4; k++) {
                        influenceOwner[i][j][k] = 0;
                        influenceValue[i][j][k] = 0;
                    }
                }
            }
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            while (nextInfluenceSyst < kMaxSystems) {
                nextInfluenceSyst += 1;
                sys = FindActiveCoLocatedSystem(nextInfluenceSyst - 1);
                if (sys != -1)
                    AddSystemInfluenceToMap(sys, &box);
            }
            if (nextInfluenceSyst == kMaxSystems) {
                nextInfluenceSyst = kMaxSystems + 1;
                InvalDialogItem(mapDialog, kMapPictItem);
            }
        }
    }
    if (event.what == DialogInput::Idle) {
        /* the wheel: one notch up presses + while it is lit, one notch down - */
        i = MouseTakeWheel();
        if (i > 0 && canZoomIn != 0) {
            *itemHit = kMapZoomInItem;
            return 1;
        }
        if (i < 0 && canZoomOut != 0) {
            *itemHit = kMapZoomOutItem;
            return 1;
        }
    }
    return 0;
}

} // namespace

void DoSystemMap(void)
{
    MapDialog().Run();
}


                                                 /* 0x004cd1a0 (mac _StrcatIndCString) */
                                                 /* 0x004b8df0 */
                                                 /* 0x00468210 */

namespace {

/* a good's price levels (kDemandLow/Medium/High, or-ed) and those under an active disaster */
struct PriceLevels {
    short levels;
    short raised;
    short lowered;
};

void DrawPiece(UI::Painter &load, const std::string &text, const RGBColor &color)
{
    load.Text(text, { UI::Style::Small, color });
}

/* A good's price levels after its name at (h, v), or indented on the next line when they do not fit the panel;
   a level under a disaster is followed by ": Higher" (green) or ": Lower" (red). The extra height used. */
short DrawPriceLevels(UI::Painter &load, const PriceLevels &p, const Rect &panel, short h, short v)
{
    static const short kLevels[] = { kDemandLow, kDemandMedium, kDemandHigh };
    std::string names[3];
    std::string higher;
    std::string lower;
    std::string text = " (";
    short extra = 0;

    if (p.levels == 0)
        return 0;
    higher = IndString(kGameStrList, kDisasterStrings);
    lower = IndString(kGameStrList, kDisasterStrings + 1);
    for (short i = 0; i < 3; i++) {
        if ((p.levels & kLevels[i]) != 0) {
            names[i] = IndString(kGameStrList, kPriceLevelStrings + i);
            if (text.size() > 2)
                text += ", ";
            text += names[i];
            bool up = (p.raised & kLevels[i]) != 0;
            bool down = (p.lowered & kLevels[i]) != 0;
            if (up || down)
                text += ": ";
            if (up)
                text += higher;
            if (up && down)
                text += "/";
            if (down)
                text += lower;
        }
    }
    text += ")";
    const char *open = " (";
    if (h + load.Measure(text, UI::Style::Small) > panel.right - 5) {
        open = "(";
        h = panel.left + 0x19;
        v += 0xc;
        extra = 0xc;
    }
    load.Pen(h, v);
    DrawPiece(load, open, rgbLightGrey);
    bool first = true;
    for (short i = 0; i < 3; i++) {
        if (names[i].empty())
            continue;
        if (!first)
            DrawPiece(load, ", ", rgbLightGrey);
        first = false;
        DrawPiece(load, names[i], rgbLightGrey);
        bool up = (p.raised & kLevels[i]) != 0;
        bool down = (p.lowered & kLevels[i]) != 0;
        if (up || down)
            DrawPiece(load, ": ", rgbLightGrey);
        if (up)
            DrawPiece(load, higher, betterRGB);
        if (up && down)
            DrawPiece(load, "/", rgbLightGrey);
        if (down)
            DrawPiece(load, lower, worseRGB);
    }
    DrawPiece(load, ")", rgbLightGrey);
    return extra;
}

/* one line of Goods Traded: the name and its price levels (none: 0); the height used */
short DrawGoodsLine(UI::Painter &load, const std::string &name, const PriceLevels &p, const Rect &panel, short v)
{
    load.Text(panel.left + 0xf, v, name, { UI::Style::Small, UI::kWhite });
    if (p.levels == 0)
        return 0xc;
    return 0xc + DrawPriceLevels(load, p, panel, panel.left + 0xf + load.Measure(UpToNul(name), UI::Style::Small), v);
}

/* a commodity's levels as the preferences show them on the map */
PriceLevels CommodityLevels(short sys, short commodity)
{
    PriceLevels p = { 0, 0, 0 };

    if (settings.mapPrices != 0) {
        p.levels = SystemPriceLevels(sys, commodity);
        if (settings.mapDisasters != 0)
            SystemDisasterLevels(sys, commodity, &p.raised, &p.lowered);
    }
    return p;
}

} // namespace

// FUNCTION: EVNOVA 0x004a51f0
// MAC: _MapDialogUpdate i386 0x0000f5d1 ppc 0x00012fe8
void MapDialogUpdate(void)
{
    std::string text;
    std::string line;
    Rect r2;
    Handle item;
    short found;
    short i;
    short j;
    short count;
    short drawn;
    short fit;
    short savedY;
    short sel;
    int32_t temp;
    short itemType;
    Rect box;
    Rect nebR;
    Rect box6;
    short st2;
    Boolean linked;
    float sy;
    short w, h;
    short xoff;
    short k;
    short n;
    short sw;
    short goodsExtra = 0;           /* the goods list's height past its room */
    const UI::TextSpec light = { UI::Style::Small, rgbLightGrey };
    const UI::TextSpec white = { UI::Style::Small, UI::kWhite };

    for (i = 0; i < kMaxSystems; i++)
        doneFlag[i] = 0;
    temp = mapScroll.h;
    savedY = mapScroll.v;

    /* the map is put together on the work page; a partial update draws it straight on the load page */
    UI::Painter load(UI::Surface::Load());
    UI::Painter work{UI::Surface(View::UIWork())};
    UI::Painter back{UI::Surface(View::UIBack())};
    UI::Painter &map = partialMapDialogUpdate != 0 ? load : work;

    load.Fill(*GetDialogPortRect(mapDialog), UI::kBlack);
    if (partialMapDialogUpdate == 0 && mapBackgroundPic != NULL)
        load.Picture(mapBackgroundPic, *GetDialogPortRect(mapDialog));
    if (partialMapDialogUpdate == 0) {
        if (mapZoom > 0.5)
            canZoomIn = 1;
        else
            canZoomIn = 0;
        if (mapZoom < 2.0)
            canZoomOut = 1;
        else
            canZoomOut = 0;
        MapButtons().Draw(load, -1);
    }
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    w = box.right - box.left;
    h = box.bottom - box.top;
    map.Fill(box, UI::kBlack);
    if (partialMapDialogUpdate == 0 && settings.showMapBorders != 0) {
        GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &r2);
        DrawMapInfluence(work, r2);
    }
    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored != 0) {
            nebR.left = (box.left + w / 2
                                + nebulaData[i].xPos / (float)mapZoom
                                - temp / mapZoom);
            nebR.top = (box.top + h / 2
                               + nebulaData[i].yPos / (float)mapZoom
                               - savedY / mapZoom);
            nebR.right = (nebR.left
                                 + nebulaData[i].width / (float)mapZoom);
            nebR.bottom = (nebR.top
                                  + nebulaData[i].height / (float)mapZoom);
            if (SectRect(&nebR, &box, &box6)) {
                fit = 0;
                for (j = 0; j < 7; j++) {
                    if (nebulaPic[i][j] != 0) {
                        QDGetPictureBounds(nebulaPic[i][j], &r2);
                        fit = j;
                        if (r2.right - r2.left >= nebR.right - nebR.left
                                || r2.bottom - r2.top >= nebR.bottom - nebR.top)
                            break;
                    }
                }
                if (nebulaPic[i][fit] != 0) {
                    SectRect(&box, &nebR, &r2);
                    if (partialMapDialogUpdate != 0) {
                        work.Picture(nebulaPic[i][fit], nebR);
                        load.Copy(UI::Surface(View::UIWork()), r2, r2);
                    } else {
                        back.Picture(nebulaPic[i][fit], nebR);
                        work.Copy(UI::Surface(View::UIBack()), r2, r2, UI::Blit::AdMax);
                    }
                }
            }
        }
    }
    DrawMap(map, box);
    if (selSystem != -1) {
        sy = systemData[selSystem].pos.v;
        r2.left = (box.left + w / 2
                          + (float)systemData[selSystem].pos.h / mapZoom
                          - temp / mapZoom);
        r2.top = (box.top + h / 2
                         + sy / mapZoom
                         - savedY / mapZoom);
        r2.right = r2.left;
        r2.bottom = r2.top;
        if (mapZoom <= 0.5)
            InsetRect(&r2, -8, -8);
        else
            InsetRect(&r2, -6, -6);
        map.Line(r2.left + 1, r2.top, r2.left + 4, r2.top, brightGreen);
        map.Line(r2.left + 1, r2.bottom, r2.left + 4, r2.bottom, brightGreen);
        map.Line(r2.right - 1, r2.top, r2.right - 4, r2.top, brightGreen);
        map.Line(r2.right - 1, r2.bottom, r2.right - 4, r2.bottom, brightGreen);
        map.Line(r2.left, r2.top + 1, r2.left, r2.top + 4, brightGreen);
        map.Line(r2.left, r2.bottom - 1, r2.left, r2.bottom - 4, brightGreen);
        map.Line(r2.right, r2.top + 1, r2.right, r2.top + 4, brightGreen);
        map.Line(r2.right, r2.bottom - 1, r2.right, r2.bottom - 4, brightGreen);
    }
    /* the names once more over DrawMap's */
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
            && (mapZoom <= 1.1 || i == selSystem)) {
            map.Text((box.left + w / 2 + systemData[i].pos.h / mapZoom
                             - temp / mapZoom + 7.0),
                     (box.top + h / 2 + systemData[i].pos.v / mapZoom
                             - savedY / mapZoom + 4.0),
                     SystName(i), white);
        }
    }
    didUpdateMap = 1;
    if (partialMapDialogUpdate != 0)
        LoadPageToDialog(mapDialog, box);
    else
        load.Copy(UI::Surface(View::UIWork()), box, box);
    if (partialMapDialogUpdate == 0) {
        GetDialogItem(mapDialog, kMapInfoItem,&itemType, &item, &box);
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &box6);
        load.Line(box6.left + 1, box.top, box6.right - 2, box.top, UI::kBlack);
        sel = selSystem;
        if (hypergateMapMode != 0
            && (selSystem == shipData->curSystem
                || selSystem == -1)) {
            text = IndString(kGameStrList, 0x134);
            // BUG (all versions): box.left + box.right / 2 is not the box's middle; the text sits right of it
            load.Text(box.left + box.right / 2 - load.Measure(text, UI::Style::Small) / 2, box.top + 0x18, text,
                      light);
        } else {
            load.Text(box.left + 10, box.top + 0xc, IndStringRaw(kGameStrList, 0x135), light);
            load.Pen(box.left + 0x2d, box.top + 0xc);
            if (systemData[sel].exploreLevel > 0) {
                short xoff;
                count = 0;
                drawn = 0;
                xoff = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[sel].navDefault[i] != -1
                        && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                        && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0)
                        count++;
                }
                if (count > 0) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (systemData[sel].navDefault[i] != -1
                            && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                            && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0) {
                            text = SpobName(systemData[sel].navDefault[i]);
                            sw = load.Measure(text, UI::Style::Small);
                            if (box.left + 0x2d + sw + xoff > box6.right - 0x14) {
                                load.Pen(box.left + 0x2d, box.top + 0x18);
                                xoff = 0;
                            } else {
                                xoff += sw;
                            }
                            load.Text(text, white);
                            drawn++;
                            if (count > drawn) {
                                load.Text(", ", white);
                                xoff += load.Measure(", ", UI::Style::Small);
                            }
                        }
                    }
                } else {
                    load.Text(IndStringRaw(kGameStrList, 0x14f), white);
                }
            } else {
                if (cheatsActive != 0) {
                    load.Text(NumText(sel + kFirstResourceID), white);
                } else {
                    load.Text(IndStringRaw(kGameStrList, 0x136), white);
                }
            }
            load.Text(box.left + 10, box.top + 0x24, IndStringRaw(kGameStrList, 0x137), light);
            load.Pen(box.left + 0x6e, box.top + 0x24);
            if (systemData[sel].exploreLevel > 0) {
                line.clear();
                for (j = 0; j < kNavSlots; j++) {
                    if (systemData[sel].navDefault[j] != -1
                        && stellarData[systemData[sel].navDefault[j]].gravity != 0) {
                        line += IndString(kGameStrList, 0x138);
                        if (systemData[sel].asteroidCount > 0
                            || systemData[sel].interference > 0
                            || systemData[sel].murk > 0)
                            line += ", ";
                        break;
                    }
                }
                if (systemData[sel].asteroidCount > 0) {
                    short roids = AsteroidReferenceCount(systemData[sel].asteroidCount);

                    if (roids <= 3)
                        line += IndString(kGameStrList, 0x139);
                    else if (roids <= 6)
                        line += IndString(kGameStrList, 0x13a);
                    else
                        line += IndString(kGameStrList, 0x13b);
                    line += " ";
                    line += IndString(kGameStrList, 0x13c);
                    if (systemData[sel].interference > 0
                        || systemData[sel].murk > 0)
                        line += ", ";
                }
                if (systemData[sel].interference > 0) {
                    if (systemData[sel].interference <= 0x21)
                        line += IndString(kGameStrList, 0x13d);
                    else if (systemData[sel].interference <= 0x42)
                        line += IndString(kGameStrList, 0x13e);
                    else
                        line += IndString(kGameStrList, 0x13f);
                    line += " ";
                    line += IndString(kGameStrList, 0x140);
                    if (systemData[sel].murk > 0)
                        line += ", ";
                }
                if (systemData[sel].murk > 0) {
                    if (systemData[sel].murk <= 0x1e)
                        line += IndString(kGameStrList, 0x141);
                    else if (systemData[sel].murk <= 0x3c)
                        line += IndString(kGameStrList, 0x142);
                    else
                        line += IndString(kGameStrList, 0x143);
                    line += " ";
                    line += IndString(kGameStrList, 0x144);
                }
                if (line.empty())
                    line = IndString(kGameStrList, 0x14f);
                if (!line.empty())
                    line[0] = (char)toupper((unsigned char)line[0]);
                load.Text(line, white);
            } else {
                if (cheatsActive != 0) {
                    load.Text(NumText(sel + kFirstResourceID), white);
                } else {
                    load.Text(IndStringRaw(kGameStrList, 0x136), white);
                }
            }
        }
        if (cheatsActive != 0)
            line = NumText(1.0 / mapZoom * 1000.0);
        else
            line = DateMedText(gameTime);
        load.Text(box.right - load.Measure(line, UI::Style::Small) - 5, box.top + 0x24, line,
                  { UI::Style::Small, rgbDarkGrey });
        didUpdateMap = 1;
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &r2);
        load.Line(r2.left + 1, r2.bottom - 1, r2.right - 2, r2.bottom - 1, UI::kBlack);
        load.Pen(r2.left + 10, r2.top + 0xc);
        linked = 0;
        if (shipData->navTargetKind == kNavTargetLink) {
            if (shipData->targetStellar == -1) {
                if (selSystem == shipData->curSystem) {
                    load.Text(IndStringRaw(kGameStrList, 0x151), light);
                } else {
                    load.Text(IndStringRaw(kGameStrList, 0x152), light);
                }
            } else {
                load.Text(IndStringRaw(kGameStrList, 0x153), light);
            }
        } else {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxStellars) {
                for (j = 0; j < kSpobHyperLinks; j++) {
                    /* BUG: i is the finished name loop's counter (0x800), so an empty link is never skipped */
                    if (stellarData[hypergateMapModeStellar].hyperLinks[j] != -i) {
                        st2 = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[j]].system;
                        if (st2 < 0 || st2 >= kMaxSystems)
                            st2 = FindSystemFromStellar(stellarData[hypergateMapModeStellar].hyperLinks[j]);
                        found = FindActiveCoLocatedSystem(st2);
                        if (found == selSystem) {
                            linked = 1;
                            break;
                        }
                    }
                }
            }
            if (selSystem == shipData->curSystem) {
                load.Text(IndStringRaw(kGameStrList, 0x151), light);
            } else if (linked != 0) {
                load.Text(IndStringRaw(kGameStrList, 0x154), light);
            } else {
                load.Text(IndStringRaw(kGameStrList, 0x152), light);
            }
        }
        load.Pen(r2.left + 0xf, r2.top + 0x1a);
        if (systemData[sel].exploreLevel <= 0) {
            if (cheatsActive != 0) {
                load.Text(NumText(sel + kFirstResourceID), white);
            } else {
                load.Text(IndStringRaw(kGameStrList, 0x136), white);
            }
        } else {
            if (cheatsActive != 0) {
                load.Text(NumText(sel + kFirstResourceID), white);
            } else {
                load.Text(SystName(sel), white);
            }
            if (SystemIsInhabited(sel) != 0) {
                load.Text(r2.left + 10, r2.top + 0x4b, IndStringRaw(kGameStrList, 0x145), light);
                load.Pen(r2.left + 0xf, r2.top + 0x58);
                count = systemData[sel].govt;
                if (count == -1) {
                    load.Text(IndStringRaw(kGameStrList, 0x14d), white);
                } else if (cheatsActive != 0) {
                    load.Text("ID ", white);
                    load.Text(NumText(count + kFirstResourceID), white);
                } else {
                    load.Text(GovtName(count), white);
                }
                load.Text(r2.left + 10, r2.top + 0x77, IndStringRaw(kGameStrList, 0x146), light);
                load.Pen(r2.left + 0xf, r2.top + 0x84);
                if (cheatsActive != 0) {
                    load.Text(NumText(playerRecord[sel]), white);
                } else {
                    load.Text(LegalStatusText(sel), white);
                }
                load.Text(r2.left + 10, r2.top + 0x9b, IndStringRaw(kGameStrList, 0x147), light);
                if (systemData[sel].exploreLevel < 2) {
                    load.Text(r2.left + 0xf, r2.top + 0xa8, IndStringRaw(kGameStrList, 0x136), white);
                } else {
                    xoff = 0;
                    for (k = 0; k <= 5; k++) {
                        if (SystemTradesGoods(sel, k) != 0)
                            xoff += DrawGoodsLine(load, cargoName[k], CommodityLevels(sel, k), r2,
                                                  r2.top + 0xa8 + xoff);
                    }
                    if (settings.mapPrices != 0) {
                        short junkLevels[kMaxJunks];
                        SystemJunkLevels(sel, junkLevels);
                        for (k = 0; k < kMaxJunks; k++) {
                            if (junkLevels[k] != 0)
                                xoff += DrawGoodsLine(load, JunkName(k), { junkLevels[k], 0, 0 }, r2,
                                                      r2.top + 0xa8 + xoff);
                        }
                    }
                    if (xoff > kGoodsHeight)
                        goodsExtra = xoff - kGoodsHeight;
                    if (xoff == 0)
                        load.Text(r2.left + 0xf, r2.top + 0xa8, IndStringRaw(kGameStrList, 0x14f), white);
                }
                load.Text(r2.left + 10, r2.top + 0xfa + goodsExtra, IndStringRaw(kGameStrList, 0x148), light);
                temp = 0;
                for (n = 0; n <= 2; n++) {
                    if (SystemHasService(sel, n) != 0)
                        temp++;
                }
                if (temp > 0) {
                    if (systemData[sel].exploreLevel < 2) {
                        load.Text(r2.left + 0xf, r2.top + 0x107 + goodsExtra, IndStringRaw(kGameStrList, 0x136),
                                  white);
                    } else {
                        xoff = 0;
                        for (j = 0; j <= 2; j++) {
                            if (SystemHasService(sel, j) != 0) {
                                load.Text(r2.left + 0xf, r2.top + 0x107 + goodsExtra + xoff,
                                          IndStringRaw(kGameStrList, j + 0x149), white);
                                xoff += 0xc;
                            }
                        }
                    }
                } else {
                    if (systemData[sel].exploreLevel >= 2) {
                        load.Text(r2.left + 0xf, r2.top + 0x106 + goodsExtra, IndStringRaw(kGameStrList, 0x14c),
                                  white);
                    } else if (systemData[sel].exploreLevel >= 1) {
                        load.Text(r2.left + 0xf, r2.top + 0x106 + goodsExtra, IndStringRaw(kGameStrList, 0x136),
                                  white);
                    }
                }
            } else {
                load.Text(r2.left + 10, r2.top + 0x4b, IndStringRaw(kGameStrList, 0x14e), white);
            }
        }
        didUpdateMap = 1;
        MapButtons().Draw(load, -1);
        LoadPageToDialog(mapDialog);
    }
}


// FUNCTION: EVNOVA 0x004a7470
// MAC: _ScrollMapDisplay i386 0x0001128d ppc 0x00014b8c
void ScrollMapDisplay(short dh, short dv)
{
    if (dh == 0 && dv == 0)
        return;
    partialMapDialogUpdate = 1;
    MapDialogUpdate();
    partialMapDialogUpdate = 0;
}

// FUNCTION: EVNOVA 0x004a7e80
// MAC: _CompactHyperRoute i386 0x0000e16a ppc 0x000118c0
void CompactHyperRoute(void)
{
    short i;

    if (hyperRoute[0] == -1) {
        for (i = 0; i < kMaxHyperRoute - 1; i++)
            hyperRoute[i] = hyperRoute[i + 1];
        hyperRoute[kMaxHyperRoute - 1] = -1;
    }
    if (hyperRoute[1] == -1) {
        for (i = 0; i < kMaxHyperRoute; i++)
            hyperRoute[i] = -1;
    }
}

// FUNCTION: EVNOVA 0x004a7fc0
// MAC: _SpoolNextHyperRouteDest i386 0x00012103 ppc 0x00015adc
void SpoolNextHyperRouteDest(void)
{
    short i;
    short group;

    if (hyperRoute[1] != -1)
        group = FindActiveCoLocatedSystem(hyperRoute[1]);
    else
        group = -1;
    if (group == FindActiveCoLocatedSystem(shipData[0].curSystem)) {
        hyperRoute[0] = -1;
        CompactHyperRoute();
    } else if (hyperRoute[1] == -1) {
        for (i = 0; i < kMaxHyperRoute; i++)
            hyperRoute[i] = -1;
    }
}

/*
 * This writes the link INDEX into +0x6c, the same field shipobj.h calls
 * targetStellar - so that word is a destination whose meaning depends on the
 * mode in +0x6a, not a stellar index outright.
 */

// FUNCTION: EVNOVA 0x004a8080
// MAC: _CueNextHyperRouteDest i386 0x0000e2dc ppc 0x00011a9c
void CueNextHyperRouteDest(void)
{
    short i;
    short sys;
    short target;
    short found;

    if ((sys = hyperRoute[1]) != -1) {
        target = FindActiveCoLocatedSystem(sys);
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (systemData[shipData[0].curSystem].links[i] != -1) {
                found = FindActiveCoLocatedSystem(systemData[shipData[0].curSystem].links[i]);
                if (found == target) {
                    shipData[0].navTargetKind = kNavTargetLink;
                    shipData[0].targetStellar = i;
                    statusRedraw[kRedrawNav] = 1;
                    break;
                }
            }
        }
    }
}

// FUNCTION: EVNOVA 0x004a8100
// MAC: _DrawMap i386 0x0000e568 ppc 0x00011df8
void DrawMap(UI::Painter &p, Rect box)
{
    FloatPoint destPt;
    FloatPoint curPt;
    RGBColor color;
    Rect sysBox;
    Rect arrowBox;
    short angle;
    short i;
    short j;
    short gateLink;
    short savedX;
    short savedY;
    short w;
    short h;
    short cur;
    short dest;
    short st;
    MapScroll from;
    MapScroll to;
    short k;
    Boolean show;
    Boolean redArrow;
    Boolean greenArrow;
    short pen;

    w = box.right - box.left;
    h = box.bottom - box.top;
    savedX = mapScroll.h;
    savedY = mapScroll.v;
    for (i = 0; i < kMaxSystems; i++)
        doneFlag[i] = 0;
    if (hypergateMapMode == 0) {
        for (i = 0; i < kMaxHyperRoute - 1; i++) {
            if (hyperRoute[i] != -1 && hyperRoute[i + 1] != -1) {
                from.h = systemData[hyperRoute[i]].pos.h;
                from.v = systemData[hyperRoute[i]].pos.v;
                to.h = systemData[hyperRoute[i + 1]].pos.h;
                to.v = systemData[hyperRoute[i + 1]].pos.v;
                p.Line((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom),
                       (box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom),
                       brightGreen, 2);
            }
        }
    }
    i = shipData->curSystem;
    for (j = 0; j < kMaxSystemLinks; j++) {
        if (systemData[i].links[j] != -1) {
            dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
            if (dest != -1) {
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                to.h = systemData[dest].pos.h;
                to.v = systemData[dest].pos.v;
                p.Line((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom),
                       (box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom),
                       hypergateMapMode != 0 ? rgbRealDarkGrey : rgbLessDarkGrey);
            }
        }
    }
    doneFlag[shipData->curSystem] = 1;
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
            && systemData[i].known != 0) {
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (systemData[i].links[j] != -1) {
                    dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
                    if (dest != -1 && doneFlag[dest] == 0) {
                        if (i == shipData->curSystem && shipData->navTargetKind == kNavTargetLink
                            && j == shipData->targetStellar)
                            color = dimGreen;
                        else if (hypergateMapMode != 0)
                            color = rgbRealDarkGrey;
                        else
                            color = rgbLessDarkGrey;
                        from.h = systemData[i].pos.h;
                        from.v = systemData[i].pos.v;
                        to.h = systemData[dest].pos.h;
                        to.v = systemData[dest].pos.v;
                        p.Line((box.left + w / 2
                                       + from.h / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2
                                       + from.v / mapZoom
                                       - savedY / mapZoom),
                               (box.left + w / 2 + to.h / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2 + to.v / mapZoom
                                       - savedY / mapZoom),
                               color);
                    }
                }
            }
            doneFlag[i] = 1;
        }
    }
    if (shipData->navTargetKind == kNavTargetLink) {
        i = shipData->curSystem;
        j = shipData->targetStellar;
        if (j >= 0 && j < kMaxSystemLinks && systemData[i].links[j] != -1) {
            dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
            if (dest != -1) {
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                to.h = systemData[dest].pos.h;
                to.v = systemData[dest].pos.v;
                p.Line((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom),
                       (box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom),
                       dimGreen);
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        redArrow = 0;
        greenArrow = 0;
        if (systemData[i].loaded != 0 && systemData[i].prepared != 0) {
            if (shipData->curSystem == i) {
                show = 1;
            } else {
                show = 0;
                if (FindActiveCoLocatedSystem(i) != -1
                    && systemData[i].exploreLevel > 0)
                    show = 1;
                if (systemData[i].known != 0)
                    show = 1;
            }
            for (k = 0; k < 32; k++) {
                if (missionSyst[k] != -1
                    && FindFirstCoLocatedSystem(i)
                       == FindFirstCoLocatedSystem(missionSyst[k])) {
                    redArrow = 1;
                    break;
                }
            }
            if (greenMissionSyst != -1
                && FindFirstCoLocatedSystem(i)
                   == FindFirstCoLocatedSystem(greenMissionSyst)) {
                greenArrow = 1;
                if (multiArrowsOnSingleSystem == 0)
                    redArrow = 0;
            }
            if (show != 0 || redArrow != 0 || greenArrow != 0) {
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                sysBox.left = (box.left + w / 2
                                      + from.h / mapZoom
                                      - savedX / mapZoom);
                sysBox.top = (box.top + h / 2 + from.v / mapZoom
                                     - savedY / mapZoom);
                sysBox.right = sysBox.left;
                sysBox.bottom = sysBox.top;
                if (mapZoom <= 0.5) {
                    pen = 2;
                    InsetRect(&sysBox, -6, -6);
                } else {
                    pen = 1;
                    InsetRect(&sysBox, -4, -4);
                }
                p.FillOval(sysBox, UI::kBlack);
                ColorCodeSystem(i, &color);
                p.Oval(sysBox, color, pen);
                if (redArrow != 0) {
                    SetRect(&arrowBox, sysBox.left - 0x10, sysBox.top - 0x10,
                            sysBox.left, sysBox.top);
                    if (redMapArrow != NULL)
                        p.Icon(redMapArrow, arrowBox);
                }
                if (greenArrow != 0) {
                    SetRect(&arrowBox, sysBox.right, sysBox.top - 0x10,
                            sysBox.right + 0x10, sysBox.top);
                    if (greenMapArrow != NULL)
                        p.Icon(greenMapArrow, arrowBox);
                }
            }
        }
    }
    cur = shipData->curSystem;
    if (cur >= 0 && cur < kMaxSystems) {
        from.h = systemData[cur].pos.h;
        from.v = systemData[cur].pos.v;
        sysBox.left = (box.left + w / 2 + from.h / mapZoom
                              - savedX / mapZoom);
        sysBox.top = (box.top + h / 2 + from.v / mapZoom
                             - savedY / mapZoom);
        sysBox.right = sysBox.left;
        sysBox.bottom = sysBox.top;
        if (mapZoom <= 0.5)
            InsetRect(&sysBox, -3, -3);
        else
            InsetRect(&sysBox, -2, -2);
        p.FillOval(sysBox, *cyanColor);
        p.Oval(sysBox, *cyanColor);
    }
    /* BUG: the upper bound tests hypergateMapMode, not the stellar, and can never fail
       (exe 0x004a921e) */
    if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
        && hypergateMapMode < kMaxSystems) {
        for (gateLink = 0; gateLink < kSpobHyperLinks; gateLink++) {
            if (stellarData[hypergateMapModeStellar].hyperLinks[gateLink] != -1) {
                st = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[gateLink]]
                         .system;
                if (st < 0 || st >= kMaxSystems)
                    st = FindSystemFromStellar(
                             stellarData[hypergateMapModeStellar].hyperLinks[gateLink]);
                dest = FindActiveCoLocatedSystem(st);
                if (dest >= 0 && dest < kMaxSystems) {
                    if (dest == selSystem) {
                        color = UI::kWhite;
                        pen = 3;
                    } else {
                        color = *cyanColor;
                        pen = 2;
                    }
                    from.h = systemData[shipData->curSystem].pos.h;
                    from.v = systemData[shipData->curSystem].pos.v;
                    to.h = systemData[dest].pos.h;
                    to.v = systemData[dest].pos.v;
                    curPt.x = (box.left + w / 2
                                      + from.h / mapZoom
                                      - savedX / mapZoom);
                    curPt.y = (box.top + h / 2 + from.v / mapZoom
                                      - savedY / mapZoom);
                    destPt.x = (box.left + w / 2 + to.h / mapZoom
                                       - savedX / mapZoom);
                    destPt.y = (box.top + h / 2 + to.v / mapZoom
                                       - savedY / mapZoom);
                    p.Line(destPt.x, destPt.y, curPt.x, curPt.y, color, pen);
                    angle = CalcInterceptAngle(&destPt, &curPt);
                    curPt = destPt;
                    Accel(angle + 45, 8.0f, &curPt);
                    p.Line(destPt.x, destPt.y, curPt.x, curPt.y, color, pen);
                    curPt = destPt;
                    Accel((angle + 315) % kFullCircle, 8.0f, &curPt);
                    p.Line(destPt.x, destPt.y, curPt.x, curPt.y, color, pen);
                    systemData[dest].known = 1;
                }
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
            && (mapZoom <= 1.1 || i == selSystem)) {
            p.Text((box.left + w / 2 + systemData[i].pos.h / mapZoom
                           - savedX / mapZoom + 7.0),
                   (box.top + h / 2 + systemData[i].pos.v / mapZoom
                           - savedY / mapZoom + 4.0),
                   FromPascal(systemData[i].name), { UI::Style::Small, UI::kWhite });
        }
    }
}


/* DrawMapInFloatingFrame (mac _DrawMapInFloatingFrame; was named ClearStarfield). */
// FUNCTION: EVNOVA 0x004a99f0
// MAC: _DrawMapInFloatingFrame i386 0x00011304 ppc 0x00014c38
void DrawMapInFloatingFrame(void)
{
    double savedZoom;

    UI::Painter p{UI::Surface(floatingMapFrame)};

    mapScroll.h = systemData[shipData[0].curSystem].pos.h;
    mapScroll.v = systemData[shipData[0].curSystem].pos.v;

    savedZoom = mapZoom;
    mapZoom = floatingMapZoom;

    p.Fill(floatingMapFrame->frameRect, UI::kBlack);
    p.Frame(floatingMapFrame->frameRect, floatingBorderColor);
    DrawMap(p, floatingMapFrame->frameRect);

    mapZoom = savedZoom;
}

/*
 * The 1.0 stored back into the time scale is the exe's two-dword constant store,
 * not an FPU one. 0x005759e8 is NOT a global -- it is the pooled literal 0.0;
 * its neighbour 0x005759e0 IS a variable despite sitting in the same pooled run.
 */

// FUNCTION: EVNOVA 0x004a9b30
// MAC: _ShowFloatingMap i386 0x00011440 ppc 0x00014d78
void ShowFloatingMap(void)
{
    short link;
    short route;

    if (floatingMapZoom <= 0.0)
        floatingMapZoom = 1.0;
    selSystem = -1;
    if (shipData->navTargetKind == kNavTargetLink) {
        route = shipData->targetStellar;
        if (route >= 0 && route < kMaxSystemLinks) {
            link = systemData[shipData->curSystem].links[route];
            if (systemData[link].loaded != 0)
                selSystem = link;
        }
    }
    hypergateMapMode = 0;
    showFloatingMap = 1;
    floatingMapStartTime = ReadTick();
    RecalcMissionSystsForMap();
    DrawMapInFloatingFrame();
}


#define kInfluenceCoarseStep 8
#define kInfluenceGridMax 255

static int bordersVisibleWidth = kMapBordersWidth;
static int bordersVisibleHeight = kMapBordersHeight;

/* RecalcMapInfluence with the border grid limited to the map item's size in grid cells. */
// BUG (CE F15.12/F16): the limit only ever shrinks, so one rebuild with a smaller rect would keep every
// later rebuild cut to it; every caller passes the map item's rect today, so it stays the dialog size
static void RecalcVisibleMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, Rect *r)
{
    short width;
    short height;

    width = (r->right - r->left) / 2;
    if (width < bordersVisibleWidth)
        bordersVisibleWidth = width;
    height = (r->bottom - r->top) / 2;
    if (height < bordersVisibleHeight)
        bordersVisibleHeight = height;
    RecalcMapInfluence(left, top, right, bottom, r);
}

// FUNCTION: EVNOVA 0x004a9bf0
// MAC: _MapPosToInfluenceCoords i386 0x000114f7 ppc 0x00014e78
void MapPosToInfluenceCoords(short x, short y, Rect *r, short *outX, short *outY)
{
    short sx;
    short sy;

    sx = ((x - mapScroll.h) / mapZoom);
    sy = ((y - mapScroll.v) / mapZoom);
    *outX = sx / 2;
    *outY = sy / 2;
    *outX += (r->right - r->left) / 2 / 2;
    *outY += (r->bottom - r->top) / 2 / 2;
}

// FUNCTION: EVNOVA 0x004a9d10
// MAC: _RecalcAllMapInfluence i386 0x00011c37 ppc 0x00015694
void RecalcAllMapInfluence(Rect *r)
{
    influenceStepSize = kInfluenceCoarseStep;
    nextInfluenceSyst = -1;
    influenceCoordsCached = 0;
    RecalcVisibleMapInfluence(0, 0, kInfluenceGridMax, kInfluenceGridMax, r);
}

// FUNCTION: EVNOVA 0x004a9d50
// MAC: _RecalcMapInfluence i386 0x000119ee ppc 0x0001545c
void RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, Rect *r)
{
    short i;
    short dy;
    short x;
    short y;
    short group;
    short syst;
    Boolean seen[kMaxSystems];

    if (influenceStepSize < 1)
        influenceStepSize = 1;

    for (x = 0; x < kMapBordersWidth; x += influenceStepSize) {
        for (y = 0; y < kMapBordersHeight; y += influenceStepSize) {
            for (group = 0; group < kInfluenceLayers; group++) {
                influenceOwner[x][y][group] = 0;
                influenceValue[x][y][group] = 0;
            }
        }
    }

    for (i = 0; i < kMaxSystems; i++)
        seen[i] = 0;

    for (i = 0; i < kMaxSystems; i++) {
        group = systemData[i].colocGroup;
        if (group != -1) {
            if (seen[group] == 0) {
                seen[group] = 1;
                syst = FindActiveCoLocatedSystem(i);
                if (syst != -1)
                    AddSystemInfluenceToMap(syst, r);
            }
        } else
            AddSystemInfluenceToMap(i, r);
    }

    if (influenceStepSize > 1) {
        for (x = 0; x < kMapBordersWidth; x += influenceStepSize) {
            for (y = 0; y < kMapBordersHeight; y += influenceStepSize) {
                for (i = 0; i < influenceStepSize; i++) {
                    for (dy = 0; dy < influenceStepSize; dy++) {
                        if (x + i < bordersVisibleWidth
                            && y + dy < bordersVisibleHeight) {
                            for (group = 0; group < kInfluenceLayers; group++) {
                                influenceOwner[x + i][y + dy][group] =
                                    influenceOwner[x][y][group];
                                influenceValue[x + i][y + dy][group] =
                                    influenceValue[x][y][group];
                            }
                        }
                    }
                }
            }
        }
    }

    influenceCoordsCached = 1;
}

#define kInfluenceSlots 4
#define kInfluenceRadius 22.0
#define kTightInfluenceRadius 11.0
#define kInfluenceMargin 12         /* cells added to the rounded radius */
#define kTightInfluenceMargin 9
#define kInfluenceFalloff 0.2
#define kTightInfluenceFalloff 0.4

// FUNCTION: EVNOVA 0x004aa070
// MAC: _AddSystemInfluenceToMap i386 0x00011592 ppc 0x00014f4c
Boolean AddSystemInfluenceToMap(short sys, Rect *r)
{
    short mapX;
    short mapY;
    short k;
    short gx;
    short gy;
    short radius;
    short govt;
    short lowestSlot;
    short lowest;
    short distX;
    short distY;
    int32_t r2;
    int32_t d2;
    int32_t level;
    unsigned char strength;
    Boolean placed;

    if (systemData[sys].loaded != 0
        && systemData[sys].known != 0
        && systemData[sys].exploreLevel > 0
        && systemData[sys].govt != -1
        && (govtData[systemData[sys].govt].flags2 & kGovtNoInfluence) == 0
        && SystemIsInhabited(sys) != 0) {
        MapPosToInfluenceCoords(systemData[sys].pos.h, systemData[sys].pos.v, r,
                                &mapX, &mapY);
        systemData[sys].influenceX = mapX;
        systemData[sys].influenceY = mapY;
        if (influenceStepSize > 1) {
            mapX -= mapX % influenceStepSize;
            mapY -= mapY % influenceStepSize;
        }

        govt = systemData[sys].govt;
        if ((govtData[govt].flags2 & kGovtMinor) != 0)
            radius = RoundD(kTightInfluenceRadius / mapZoom) + kTightInfluenceMargin;
        else
            radius = RoundD(kInfluenceRadius / mapZoom) + kInfluenceMargin;
        if (influenceStepSize > 1)
            radius -= radius % influenceStepSize;
        r2 = radius * radius;

        if (mapX >= -radius && mapX <= radius + bordersVisibleWidth
            && mapY >= -radius && mapY <= radius + bordersVisibleHeight) {
            for (gx = mapX - radius; gx <= mapX + radius; gx += influenceStepSize) {
                for (gy = mapY - radius; gy <= mapY + radius;
                     gy += influenceStepSize) {
                    if (gx >= 0 && gx < bordersVisibleWidth
                        && gy >= 0 && gy < bordersVisibleHeight) {
                        distX = gx - mapX;
                        distY = gy - mapY;
                        if (distX < 0)
                            distX *= -1;
                        if (distY < 0)
                            distY *= -1;
                        d2 = distX * distX + distY * distY;
                        if (d2 > r2)
                            continue;

                        if ((govtData[govt].flags2 & kGovtMinor) != 0)
                            level = ((r2 - d2) * kTightInfluenceFalloff
                                              * mapZoom);
                        else
                            level = ((r2 - d2) * kInfluenceFalloff * mapZoom);
                        if (level < 1)
                            level = 1;
                        if (level > 255)
                            level = 255;
                        strength = level;

                        placed = 0;
                        for (k = 0; k < kInfluenceSlots; k++) {
                            if (influenceValue[gx][gy][k] != 0) {
                                if (GovtAllies(govt, influenceOwner[gx][gy][k]) != 0) {
                                    if (strength
                                        > influenceValue[gx][gy][k]) {
                                        influenceOwner[gx][gy][k] = govt;
                                        influenceValue[gx][gy][k] =
                                            strength;
                                    }
                                    placed = 1;
                                    break;
                                }
                            }
                        }
                        if (placed == 0) {
                            for (k = 0; k < kInfluenceSlots; k++) {
                                if (influenceValue[gx][gy][k] == 0) {
                                    influenceValue[gx][gy][k] = strength;
                                    influenceOwner[gx][gy][k] = govt;
                                    placed = 1;
                                    break;
                                }
                            }
                        }
                        if (placed == 0) {
                            lowest = 0;
                            lowestSlot = 3;
                            for (k = 0; k < kInfluenceSlots; k++) {
                                if (lowest == 0 || influenceValue[gx][gy][k] < lowest) {
                                    lowest = influenceValue[gx][gy][k];
                                    lowestSlot = k;
                                }
                            }
                            if (lowest != 0) {
                                influenceValue[gx][gy][lowestSlot] =
                                    strength;
                                influenceOwner[gx][gy][lowestSlot] = govt;
                            }
                        }
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004aa620
// MAC: _DrawMapInfluence i386 0x0000e360 ppc 0x00011b5c
void DrawMapInfluence(UI::Painter &p, Rect bounds)
{
    Rect cell;
    short x;
    short k;
    unsigned char best;
    unsigned char v;
    unsigned char govt;
    RGBColor color;
    short y;

    if (influenceStepSize < 1)
        influenceStepSize = 1;

    for (x = 0; x < kMapBordersWidth; x += influenceStepSize) {
        for (y = 0; y < kMapBordersHeight; y += influenceStepSize) {
            if (influenceValue[x][y][0] == 0)
                continue;

            cell.left = x * 2 + bounds.left;
            cell.right = cell.left + influenceStepSize * 2;
            cell.top = y * 2 + bounds.top;
            cell.bottom = cell.top + influenceStepSize * 2;

            if (cell.right < bounds.left)
                continue;
            if (cell.left > bounds.right)
                continue;
            if (cell.bottom < bounds.top)
                continue;
            if (cell.top > bounds.bottom)
                continue;

            if (cell.left < bounds.left)
                cell.left = bounds.left;
            if (cell.right > bounds.right)
                cell.right = bounds.right;
            if (cell.top < bounds.top)
                cell.top = bounds.top;
            if (cell.bottom > bounds.bottom)
                cell.bottom = bounds.bottom;

            best = 0;
            for (k = 0; k < kInfluenceLayers; k++) {
                v = influenceValue[x][y][k];
                if (v > 255)
                    v = 255;
                if (v > best) {
                    color.red = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.red);
                    color.green = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.green);
                    color.blue = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.blue);
                    best = v;
                }
            }

            p.Fill(cell, color);
        }
    }
}
