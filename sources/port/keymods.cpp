/* keymods.cpp -- the modifier-key scan the port has to synthesise (win 0x004cea20). Notes: Research/notes/keymods.md */
#include "macport.h"

// FUNCTION: EVNOVA 0x004cea20
int32_t GetKeyModifiers(void)
{
    int32_t mods;

    mods = 0;
    if (KeyIsDown(kScanLShift) || KeyIsDown(kScanRShift))
        mods |= shiftKey;
    if (KeyIsDown(kScanLCtrl) || KeyIsDown(kScanRCtrl))
        mods |= controlKey;
    if (KeyIsDown(kScanLAlt) || KeyIsDown(kScanRAlt))
        mods |= optionKey;
    return mods;
}
