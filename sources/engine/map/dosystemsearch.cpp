/* dosystemsearch.cpp -- DoSystemSearch; exe 0x004aab30..0x004aaf50. Notes: Research/notes/mapdlg.md */

#include "sound.h"
#include "game.h"
#include "render.h"
#include "dialogs.h"
#include "dialog.h"
#include "stellar.h"

/* the map object's globals */
Image_t *redMapArrow;
Image_t *greenMapArrow;
void *mapCursorSwitchRgn;
PicHandle nebulaPic[32][7];
PicHandle mapBackgroundPic;
Boolean partialMapDialogUpdate;
Boolean didUpdateMap;
Boolean canZoomIn;
Boolean canZoomOut;
Boolean showClearRouteButton;
Boolean multiArrowsOnSingleSystem;
Boolean doneFlag[2048];
Boolean influenceCoordsCached;
short missionSyst[32];
short greenMissionSyst;
short selSystem;
short influenceStepSize;
short nextInfluenceSyst;
DialogPtr mapDialog;
unsigned char influenceValue[kBordersWidth][kBordersHeight][4];
unsigned char influenceOwner[kBordersWidth][kBordersHeight][4];

#define kSearchDlogID 0xbbd
#define kSearchTextItem 5

// FUNCTION: EVNOVA 0x004aab30
// MAC: _DoSystemSearch i386 0x00011c86 ppc 0x000156dc
void DoSystemSearch(void)
{
    short itemHit;
    short itemType;
    Handle item;
    Rect box;
    Dialog dialog;
    DialogPtr dlg;
    Boolean searchNow;
    unsigned char rawText[256];
    unsigned char typedText[256];
    unsigned char candText[256];
    short i;
    short c;
    short runLen;
    short matchCount;
    short bestRun;
    short bestSystem;
    short bestLen;
    char ch;

    dlg = dialog.Open(kSearchDlogID);
    if (!dialog.IsOpen())
        return;

    SetDialogDefaultItem(dlg, 1);
    SetDialogGWorld(dlg);

    GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
    SetDialogItemText((Handle)item, (const unsigned char *)"\p");
    SelectDialogItemText(dlg, kSearchTextItem, 0, 1);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();

    rawText[0] = 0;
    searchNow = 0;
    do {
        itemHit = dialog.Modal();
        if (itemHit == 1) {
            GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
            GetDialogItemText((Handle)item, rawText);
            searchNow = 1;
        }
        if (itemHit == 3) {
            searchNow = 1;
        }
    } while (!searchNow);

    dialog.Close();
    RedrawDialog(mapDialog);

    if (rawText[0] == 0)
        return;

    bestRun = -1;
    bestSystem = -1;
    bestLen = 0;
    matchCount = 0;
    typedText[0] = 0;
    for (c = 0; c < rawText[0]; c++) {
        ch = tolower(rawText[c + 1]);
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            typedText[0]++;
            typedText[typedText[0]] = ch;
        }
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded == 0)
            continue;
        if (systemData[i].exploreLevel <= 0)
            continue;
        if (systemData[i].known == 0)
            continue;

        candText[0] = 0;
        c = 0;
        while (c < (char)systemData[i].name[0]) {
            ch = tolower((char)systemData[i].name[c + 1]);
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
                candText[0]++;
                candText[candText[0]] = ch;
            }
            c++;
        }

        runLen = 0;
        for (c = 0; c < typedText[0]; c++) {
            if (c >= candText[0])
                break;
            if (typedText[c + 1] != candText[c + 1])
                break;
            runLen++;
        }

        if (runLen <= 0)
            continue;

        matchCount++;
        if (runLen > bestRun || (runLen == bestRun && candText[0] < bestLen)
            || bestSystem == -1) {
            bestSystem = i;
            bestRun = runLen;
            bestLen = candText[0];
        }
    }

    if (bestSystem != -1 && (bestRun > 1 || matchCount == 1)) {
        selSystem = bestSystem;
        mapScroll.h = systemData[bestSystem].pos.h;
        mapScroll.v = systemData[bestSystem].pos.v;
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}
