/* newpilotdlg.cpp -- the New Pilot confirm/abort gate (win 0x0048a7e0). Notes: Research/notes/newpilotdlg.md */

#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "dialog.h"

#define kNewPilotDlogID          0xc1d
#define kNewPilotOneCharDlogID   0xc1e
#define kItemOK                      1
#define kItemCancel                  2
#define kItemStrictPlay              4
#define kItemFirstNameField          8
#define kItemGenderPopup            11
#define kItemCharPopup              13
#define kNameFieldCount              2
#define kNameFieldStride         0x100
#define kMaxPilotNameLen          0x18
#define kSelectAllNameText        0xfe


// FUNCTION: EVNOVA 0x0048a7e0
// MAC: _DoNewPilotNameDialog i386 0x000165cc ppc 0x0001a3ec
Boolean DoNewPilotNameDialog(char *strictPlay)
{
    short resID;
    uint32_t resType;
    Str255 name;
    int32_t selected;
    int32_t idx;
    short itemHit;
    Boolean result;
    Boolean done;
    Rect box;
    short type;
    Boolean strict;
    short n;
    Str255 savedName;
    void *h;
    ControlPtr ctl;
    Dialog dialog;
    DialogPtr dlg;
    unsigned char *p;
    short i;
    Handle res;
    short count;

    done = 0;
    strict = *strictPlay;
    n = CountResources(kCharType);
    if (n > 1) {
        count = 0;
        for (i = 0; i < n; i++) {
            res = GetIndResource(kCharType, (i + 1));
            if (res != NULL) {
                GetResInfo(res, &resID, &resType, savedName);
                if ((int32_t)savedName[1] != '.')
                    count++;
                ReleaseResource(res);
            }
        }
    } else {
        count = 0;
    }
    dlg = dialog.Open(count > 1 ? kNewPilotDlogID : kNewPilotOneCharDlogID);
    if (!dialog.IsOpen())
        return 0;

    SetDialogDefaultItem(dlg, 1);
    FrontWindow(dlg);
    SetDialogGWorld(dlg);
    DrawDialog(dlg);

    GetDialogItem(dlg, kItemStrictPlay, &type, (Handle *)&h, &box);
    ctl = (ControlPtr)h;
    if (strict != 0)
        SetControlValue(ctl, 1);
    else
        SetControlValue(ctl, 0);

    for (i = 0, p = newPlayerName[0]; i < kNameFieldCount; i++) {
        GetDialogItem(dlg, i + kItemFirstNameField, &type, (Handle *)&h, &box);
        SetDialogItemText((Handle)h, p);
        p += kNameFieldStride;
    }
    SelectDialogItemText(dlg, kItemFirstNameField, 0, kSelectAllNameText);
    FlushEvents(everyEvent - diskMask, 0);

    GetDefaultCharResourceName(savedName);
    selected = 1;
    if (savedName[0] != 0) {
        PtoCstr(savedName);
        n = CountResources(kCharType);
        if (n > 1) {
            i = idx = 0;
            if (n > 0) do {
                res = GetIndResource(kCharType, (i + 1));
                if (res != NULL) {
                    GetResInfo(res, &resID, &resType, name);
                    if ((int32_t)name[1] != '.') {
                        idx++;
                        PtoCstr(name);
                        if (strcmp((char *)name, (char *)savedName) == 0) {
                            selected = idx;
                            selected++;
                            // BUG: the break skips ReleaseResource(res) -- one ch�r resource leaks per dialog
                            break;
                        }
                    }
                    ReleaseResource(res);
                }
            } while (++i < n);
        }
    }
    GetDialogItem(dlg, kItemCharPopup, &type, (Handle *)&h, &box);
    ctl = (ControlPtr)h;
    SetControlValue(ctl, (short)selected);

    do {
        itemHit = dialog.Modal();
        if (itemHit == kItemOK) {
            result = 1;
            done = 1;
            for (n = 0; n < kNameFieldCount; n++) {
                GetDialogItem(dlg, n + kItemFirstNameField, &type, (Handle *)&h, &box);
                GetDialogItemText((Handle)h, savedName);
                PtoCstr(savedName);
                if (strlen((char *)savedName) > kMaxPilotNameLen) {
                    SysBeep(0);
                    result = 0;
                    done = 0;
                    SelectDialogItemText(dlg, n + kItemFirstNameField, 0,
                                         kMaxPilotNameLen - 1);
                    break;
                }
            }
        }
        if (itemHit == kItemStrictPlay) {
            if (strict != 0)
                strict = 0;
            else
                strict = 1;
            GetDialogItem(dlg, kItemStrictPlay, &type, (Handle *)&h, &box);
            ctl = (ControlPtr)h;
            if (strict != 0)
                SetControlValue(ctl, 1);
            else
                SetControlValue(ctl, 0);
        }
        if (itemHit == kItemCancel) {
            result = 0;
            done = 1;
        }
    } while (done == 0);

    if (result != 0) {
        for (i = 0, p = newPlayerName[0]; i < kNameFieldCount; i++) {
            GetDialogItem(dlg, i + kItemFirstNameField, &type, (Handle *)&h, &box);
            GetDialogItemText((Handle)h, p);
            p += kNameFieldStride;
        }
        *strictPlay = strict;
        GetDialogItem(dlg, kItemCharPopup, NULL, (Handle *)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[0]);
        GetDialogItem(dlg, kItemGenderPopup, NULL, (Handle *)&ctl, NULL);
        GetMenuItemText(ctl, GetControlValue(ctl), newPilotPopupChoice[1]);
    }
    SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
    SWSetPortToWindow(gSpriteWorld);
    return result;
}
