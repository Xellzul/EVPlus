/* offeronemission.cpp -- the mission briefing/offer dialog */

#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "crashlog.h"
#include "missions.h"
#include "render.h"
#include "stellar.h"
#include "gamedialog.h"
#include "buttons.h"
#include "scrolltext.h"
#include "painter.h"
#include "text.h"

#define kWideDlogID               0x03fc
#define kNarrowDlogID             0x03f8
#define kWidePictID               0x2150
#define kNarrowPictBase           0x2149
#define kMissionPictCount              3

#define kStrAcceptRefusable           50
#define kStrAcceptOnly                27
#define kStrRefuse                    51
#define kButtonPadding                40
#define kMinButtonWidth              100
#define kMinTextHeight              0x30
#define kTextMargin                   16

#define kItemRefuse                    2
#define kItemBriefText                 3
#define kBriefItemMap                  4
#define kBriefItemPlayerInfo           5
#define kItemDivider                   6
#define kBriefItemMissionInfo          7
#define kItemScrollUp                  9
#define kItemScrollDown               10

/* the briefing text, its scroll buttons and SingleMissionButtons' slots for them */
constexpr ScrollingText::Layout kOfferTextLayout = { kItemBriefText, kItemScrollUp, kItemScrollDown, 2, 3, 10, 250 };

static ScrollingText OfferText()
{
    return ScrollingText(missionDialog, offerTextHandle, kOfferTextLayout);
}

namespace {

// FUNCTION: EVNOVA 0x00442510
// MAC: _OfferOneMission i386 0x000a21ac ppc 0x000adfd8
/* A mission briefing to accept or, unless the mission says otherwise, refuse. */
class SingleMissionDialog final : public GameDialog {
public:
    short Run(short offered);   /* 1 accepted, 0 refused, invalid or not opened, else -1 */

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    short mission;
    Boolean refused = 0;
    Boolean accepted = 0;
    short refuseDescID;     /* the desc shown on refusal, -1 none */
    char setString[256];    /* the bit-set expression run on refusal */
    Boolean savedUp;        /* the scroll buttons' last drawn state */
    Boolean savedDown;
};

short SingleMissionDialog::Run(short offered)
{
    Rect box;
    int32_t missionResID;
    void *itemHandle;
    short itemType;
    Rect *wr;
    short i;
    short itemHeight;
    short shift;
    short floatZero;
    short widest;
    short otherWidth;
    short shown;
    unsigned char labelChar;
    Picture backdrops[kMissionPictCount];

    mission = offered;
    if (mission < 0 || mission >= kMaxMissions)
        return 0;

    if ((missionAvailData[mission].flags & kMissionCantRefuse) != 0)
        canRefuseMission = 0;
    else
        canRefuseMission = 1;

    crashlog_print("offering mission ");
    crashlog_print_pstr(missionName[mission]);
    crashlog_print(" (");
    missionResID = mission + kFirstResourceID;
    crashlog_print_num(missionResID);
    crashlog_print(")\n");

    strncpy(briefingTextPtr, "", 3);
    LoadDescription(briefingTextPtr, mission + kMissionDescBase);

    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, descFlags & kDescMovieDoubled,
                    descFlags & kDescMovieFade);

    if (screenFaded != 0) {
        UpdateAllPortDialogs();
        RefreshSAT();
        RedrawStatus(1);
        FadeScreenIn();
        LoadDescription(briefingTextPtr, mission + kMissionDescBase);
    }

    if (canRefuseMission == 0 && strlen(briefingTextPtr) == 0) {
        accepted = DoMissionAccept(mission);
        if (accepted != 0)
            return 1;
        return -1;
    }

    if (strlen(briefingTextPtr) < sizeof briefingTextPtr - 1)
        strcat(briefingTextPtr, "\r");

    dialog.Open(descResourcePictId >= 128 ? kWideDlogID : kNarrowDlogID, SingleMissionDialogUpdate,
                Filter, &missionDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return 0;

    for (i = 0; i < kMissionPictCount; i++)
        backdrops[i].Bind(missionBackgroundPic[i]);
    if (descResourcePictId >= 128) {
        backdrops[0].Load(kWidePictID);
        missionBackgroundPic[1] = NULL;
        missionBackgroundPic[2] = NULL;
    } else {
        for (i = 0; i < kMissionPictCount; i++)
            backdrops[i].Load(kNarrowPictBase + i);
    }

    MungeBriefing(1, mission);

    GetDialogItem(missionDialog, kItemBriefText, &itemType, (Handle *)&itemHandle, &box);
    offerTextHandle = TENew(&box, &box);
    if (offerTextHandle != NULL) {
        (*offerTextHandle)->txFont = smallTextFont;
        (*offerTextHandle)->txSize = smallTextSize;
        UI::Painter(UI::Surface::Dialog(missionDialog))
            .EditSetText(offerTextHandle, briefingTextPtr, strlen(briefingTextPtr), UI::Style::Small);

        shown = TETextHeight(offerTextHandle);
        itemHeight = box.bottom - box.top;
        if (shown < itemHeight) {
            if (descResourcePictId < 128) {
                shown = shown < kMinTextHeight
                             ? (short)kMinTextHeight : shown;
                wr = GetDialogPortRect(missionDialog);
                box.top = wr->top;
                box.left = wr->left;
                box.bottom = wr->bottom;
                box.right = wr->right;
                shift = itemHeight - shown - kTextMargin;
                box.bottom -= shift;
                SizeDialogWindow(missionDialog, box.right - box.left,
                             box.bottom - box.top, 1);
                floatZero = 0;
                floatZero += shift * 0.3;
                OffsetDialogWindow(missionDialog, 0, floatZero, 1);

                GetDialogItem(missionDialog, kItemAccept, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemAccept, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemRefuse, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemRefuse, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemDivider, &itemType,
                              (Handle *)&itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemDivider, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemBriefText, &itemType,
                              (Handle *)&itemHandle, &box);
                box.bottom -= shift;
                SetDialogItem(missionDialog, kItemBriefText, itemType,
                              itemHandle, &box);
            }
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          (Handle *)&itemHandle, &(*offerTextHandle)->destRect);
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          (Handle *)&itemHandle, &(*offerTextHandle)->viewRect);
            UI::Painter(UI::Surface::Dialog(missionDialog)).EditMeasure(offerTextHandle, UI::Style::Small);
        }
    }

    strcpy((char *)missionButtonLabel[0], "");
    strcpy((char *)missionButtonLabel[1], "");

    missionFileHand = NULL;
    missionFileHand = GetResource(kMisnType, mission + kFirstResourceID);
    if (missionFileHand != NULL) {
        CopyCField(missionButtonLabel[0], ((MisnRes *)*missionFileHand)->acceptButton);
        CopyCField(missionButtonLabel[1], ((MisnRes *)*missionFileHand)->refuseButton);
        for (i = 0; i <= kMissionButtonLabelCount - 1; i++) {
            labelChar = missionButtonLabel[i][0];
            if (tolower(HeldChar(labelChar, missionButtonLabel[i][0])) < 'a'
                    || tolower(HeldChar(labelChar, missionButtonLabel[i][0])) > 'z')
                strcpy((char *)missionButtonLabel[i], "");
        }
        refuseDescID = ((MisnRes *)*missionFileHand)->refuseText;
        CopyCField(setString, ((MisnRes *)*missionFileHand)->onRefuse);
        ReleaseResource(missionFileHand);
    } else {
        refuseDescID = -1;
        strcpy(setString, "");
    }

    if (strlen((char *)missionButtonLabel[0]) == 0) {
        if (canRefuseMission != 0)
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptRefusable);
        else
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptOnly);
    } else {
        CtoPstr((char *)missionButtonLabel[0]);
    }
    if (strlen((char *)missionButtonLabel[1]) == 0)
        GetIndString(missionButtonLabel[1], kButtonStrList, kStrRefuse);
    else
        CtoPstr((char *)missionButtonLabel[1]);

    {
        UI::Painter p(UI::Surface::Dialog(missionDialog));
        widest = p.Measure(FromPascal(missionButtonLabel[0]), UI::Style::System);
        otherWidth = p.Measure(FromPascal(missionButtonLabel[1]), UI::Style::System);
    }
    if (widest > otherWidth)
        otherWidth = widest;
    otherWidth += kButtonPadding;
    if (otherWidth < kMinButtonWidth)
        otherWidth = kMinButtonWidth;

    GetDialogItem(missionDialog, kItemAccept, &itemType, (Handle *)&itemHandle, &box);
    box.right = box.left + otherWidth;
    SetDialogItem(missionDialog, kItemAccept, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemRefuse, &itemType, (Handle *)&itemHandle, &box);
    box.left = box.right - otherWidth;
    SetDialogItem(missionDialog, kItemRefuse, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemDivider, &itemType, (Handle *)&itemHandle, &box);
    box.left = box.right = (box.left + box.right) / 2;
    InsetRect(&box, -otherWidth / 2, 0);
    SetDialogItem(missionDialog, kItemDivider, itemType, itemHandle, &box);

    lastTextScrollTime = ReadTick();
    OfferText().ProbeRange();
    savedUp = canScrollTextUp;
    savedDown = canScrollTextDown;

    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(missionDialog);
    SingleMissionDialogUpdate();

    Loop();

    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) != 0)
        PlayQTMovie(descMovieName, descFlags & kDescMovieDoubled,
                    descFlags & kDescMovieFade);

    for (i = 0; i < kMissionPictCount; i++)
        backdrops[i].Release();
    if (offerTextHandle != NULL)
        TEDispose(offerTextHandle);
    while (KeyCheck(keyPrefs.missions))
        BusyIdle();

    dialog.Close();
    if (screenFaded != 0) {
        if (playerIsLanded != 0)
            UpdateAllPortDialogs();
        RedrawStatus(1);
        FadeScreenIn();
    }
    FlushEvents(7, 0);
    if (refused != 0)
        return 0;
    if (accepted != 0)
        return 1;
    return -1;
}

/* After a sub-dialog closed over the offer: the spaceport when landed, then the offer. Not UpdateAllPortDialogs:
   it would pick the mission BBS update for missionDialog after the BBS was closed. */
static void RedrawOfferAfterSubDialog()
{
    if (portDialog != NULL) {
        RedrawDialog(portDialog);
        PortDialogUpdate();
    }
    RedrawDialog(missionDialog);
}

void SingleMissionDialog::Hit(short item)
{
    if (canRefuseMission != 0) {
        if (item == kItemAccept) {
            FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            SetDialogVisible(missionDialog, 0);
            accepted = DoMissionAccept(mission);
            SingleMissionDialogUpdate();
            LoadDescription(briefingTextPtr, mission + kMissionDescBase);
            MungeBriefing(1, mission);
            done = 1;
        }
        if (item == kItemRefuse) {
            if (refuseDescID != -1) {
                SetDialogVisible(missionDialog, 0);
                strncpy(briefingTextPtr, "", 3);
                LoadDescription(briefingTextPtr, refuseDescID);
                MungeBriefing(0, -1);
                CopyBriefingText(textShowString, briefingTextPtr);
                ShowTextDialog(0, 1);
            }
            crashlog_mission_bit_set("refusing mission", NULL, 0,
                                     mission + kFirstResourceID, setString);
            EvalMissionBitSetString(setString);
            refused = 1;
            done = 1;
        }
    } else if (item == kItemAccept || item == kItemRefuse) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        accepted = DoMissionAccept(mission);
        LoadDescription(briefingTextPtr, mission + kMissionDescBase);
        MungeBriefing(1, mission);
        SingleMissionDialogUpdate();
        done = 1;
    }
    if (item == kBriefItemMap) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        if ((missionAvailData[mission].flags & kMissionBriefingArrow) != 0) {
            if (briefMissionInfo[mission].destStellar != -1) {
                greenMissionSyst =
                    stellarData[briefMissionInfo[mission].destStellar].system;
            } else {
                if (briefMissionInfo[mission].returnStellar != -1)
                    greenMissionSyst =
                        stellarData[briefMissionInfo[mission].returnStellar].system;
            }
        }
        OpenMapKeepingNav();
        greenMissionSyst = -1;
        RedrawOfferAfterSubDialog();
    }
    if (item == kBriefItemPlayerInfo) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        DoPlayerInfoDialog();
        RedrawOfferAfterSubDialog();
    }
    if (item == kBriefItemMissionInfo && AnyMissionShown()) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        DoMissionInfoDialog();
        LoadDescription(briefingTextPtr, mission + kMissionDescBase);
        MungeBriefing(1, mission);
        RedrawOfferAfterSubDialog();
    }
    OfferText().Step(item);
    OfferText().InvalidateChanged(savedUp, savedDown);
}

static constexpr short kSingleMissionSlotItems[] = { kItemAccept, kItemRefuse, -1, -1 };

// FUNCTION: EVNOVA 0x00447170
// MAC: _SingleMissionFilter i386 0x00098d9a ppc 0x000a4984
Boolean SingleMissionDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;
    int acceptKey;
    int refuseKey;
    EventChar c;
    DialogInput what;

    if (QuickKeyHit(kBriefItemMap, kBriefItemPlayerInfo, kBriefItemMissionInfo, itemHit))
        return 1;
    what = ev.what;
    if (ev.what == DialogInput::Key) {
        c = ev.ch;
        c = tolower(c);
        if (IsReturnChar(c)) {
            *itemHit = kItemAccept;
            return 1;
        }
        refuseKey = missionButtonLabel[1][1];
        acceptKey = missionButtonLabel[0][1];
        if (tolower(acceptKey) != tolower(refuseKey)) {
            if (c == tolower(acceptKey)) {
                *itemHit = kItemAccept;
                return 1;
            }
            if (canRefuseMission != 0 && c == tolower(refuseKey)) {
                *itemHit = kItemRefuse;
                return 1;
            }
        }
        if (OfferText().Key(c, itemHit))
            return 1;
    }
    if (what == DialogInput::Click) {
        where = ev.where;
        SingleMissionButtons buttons;
        if (OfferText().Press(where, itemHit, buttons))
            return 1;
        SingleMissionDialogUpdate();
        return ButtonItemHit(buttons.Track(where), kSingleMissionSlotItems, itemHit);
    }
    if (what == DialogInput::Idle && OfferText().Wheel(itemHit))
        return 1;
    return 0;
}

} // namespace

short OfferOneMission(short mission)
{
    return SingleMissionDialog().Run(mission);
}

// FUNCTION: EVNOVA 0x00447680
// MAC: _SingleMissionDialogUpdate i386 0x00098950 ppc 0x000a45e0
void SingleMissionDialogUpdate(void)
{
    PicHandle loaded;
    Rect r;
    short itemType;
    void *item;
    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(missionDialog), UI::kBlack);
    DrawDialogBackground(p, missionBackgroundPic, *GetDialogPortRect(missionDialog),
                         descResourcePictId >= kFirstResourceID);
    SingleMissionButtons().Draw(p, -1);
    if (descResourcePictId >= kFirstResourceID) {
        GetDialogItem(missionDialog, 8, &itemType, (Handle *)&item, &r);
        loaded = GetPicture(descResourcePictId);
        if (loaded != NULL) {
            p.Picture(loaded, r);
            QDReleaseResource(loaded);
        }
    }
    GetDialogItem(missionDialog, kItemBriefText, &itemType, (Handle *)&item, &r);
    p.EditDraw(offerTextHandle, r, { UI::Style::Small, UI::kWhite }, UI::kBlack);
    LoadPageToDialog(missionDialog);
}
