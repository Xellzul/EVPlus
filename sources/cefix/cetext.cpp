/* cetext.cpp -- EV Nova CE: the version text on the main menu. (CE's other text change, Mac Roman text
   drawn as Unicode in place of iconv's CP1252, is in port/qdtext.cpp.) Notes: Research/cefix-plan.md */

#include "render.h"
#include "cefix.h"

#define kVersionText        "EV (plus) Nova Community Edition r4 "
#define kVersionGrey        0x8000
#define kVersionLeft        5
#define kVersionRaise       6           /* baseline above the backdrop's bottom edge */
#define kVersionTextSize    9

/* Grey, in the lower-left corner of the main menu's backdrop. */
void CE_DrawVersion(void)
{
    static const char version[] = kVersionText;

    CurrentGrafPort->foreColor.red = kVersionGrey;
    CurrentGrafPort->foreColor.green = kVersionGrey;
    CurrentGrafPort->foreColor.blue = kVersionGrey;
    MoveTo(kVersionLeft, CurrentGrafPort->pix.bounds.bottom - kVersionRaise);
    CurrentGrafPort->txFont = smallTextFont;
    TextSize(kVersionTextSize);
    DrawTextRun(version, sizeof(version) - 1, NULL, 0, 0);
}
