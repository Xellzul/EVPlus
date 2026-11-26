/* capturedlg.cpp -- the captured-ship assignment dialog */

#include "game.h"
#include "prototypes.h"
#include "render.h"
#include "sound.h"
#include "gamedialog.h"
#include "buttons.h"
#include "painter.h"
#include "text.h"

DialogPtr captureDialog;
PicHandle captureBackgroundPic;

#define kCaptureStrIdx  0x76

#define kCaptureDlogID   0x3fa
#define kCapturePictID   0x2144

namespace {

// FUNCTION: EVNOVA 0x00497eb0
// MAC: _DoCapturedShipAssignmentDialog i386 0x0005b508 ppc 0x000635a8
/* Asks whether a captured ship becomes the flagship or an escort. */
class CapturedAssignmentDialog final : public GameDialog {
public:
    CaptureUse Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    CaptureUse result = CaptureUse::None;
};

CaptureUse CapturedAssignmentDialog::Run()
{
    dialog.Open(kCaptureDlogID, CapturedAssignmentDialogUpdate, Filter, &captureDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return CaptureUse::Escort;
    Picture backdrop(captureBackgroundPic);
    backdrop.Load(kCapturePictID);
    CapturedAssignmentDialogUpdate();
    FlushEvents(everyEvent - diskMask, 0);
    Loop();
    dialog.Close();
    backdrop.Release();
    return result;
}

void CapturedAssignmentDialog::Hit(short item)
{
    if (item == 1) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        result = CaptureUse::Flagship;
        done = 1;
    }
    if (item == 2) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        result = CaptureUse::Escort;
        done = 1;
    }
}

constexpr KeyItem kCapAssignKeys[] = { { 'm', 1 }, { 'e', 2 } };

// FUNCTION: EVNOVA 0x00498040
// MAC: _CapturedAssignmentFilter i386 0x00051782 ppc 0x0005a024
Boolean CapturedAssignmentDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;
    EventChar key;
    DialogInput what;

    what = ev.what;
    if (what == DialogInput::Key) {
        key = ev.ch;
        key = tolower(key);
        if (KeyItemHit(key, kCapAssignKeys, itemHit))
            return 1;
    }
    if (what == DialogInput::Click) {
        where = ev.where;
        CapturedAssignmentDialogUpdate();
        return ButtonItemHit(CapAssignButtons().Track(where), itemHit);
    }
    return 0;
}

} // namespace

CaptureUse DoCapturedShipAssignmentDialog(void)
{
    return CapturedAssignmentDialog().Run();
}

// FUNCTION: EVNOVA 0x00498110
// MAC: _CapturedAssignmentDialogUpdate i386 0x0005157d ppc 0x00059e90
void CapturedAssignmentDialogUpdate(void)
{
    Rect box;
    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(captureDialog), UI::kBlack);
    if (captureBackgroundPic != NULL)
        p.Picture(captureBackgroundPic, *GetDialogPortRect(captureDialog));
    GetDialogItem(captureDialog, 3, NULL, NULL, &box);
    p.TextBox(box, IndString(kGameStrList, kCaptureStrIdx), { UI::Style::Small, UI::kWhite }, UI::kBlack);
    CapAssignButtons().Draw(p, -1);
    LoadPageToDialog(captureDialog);
}
