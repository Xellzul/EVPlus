/* dialog.cpp -- Dialog, Picture and CopyLoadPortToDialog */
#include "dialog.h"
#include "dialogs.h"

void Picture::Release()
{
    if (slot != nullptr && *slot != NULL) {
        QDReleaseResource(*slot);
        *slot = NULL;
    }
}

void CopyLoadPortToDialog(DialogPtr dlg)
{
    CopyLoadPortToDialog(dlg, *GetDialogPortRect(dlg));
}

void CopyLoadPortToDialog(DialogPtr dlg, const Rect &box)
{
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(dlg);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(GetDialogPort(dlg)),
             &box, &box, srcCopy, NULL);
}

DialogPtr Dialog::Opened(DialogPtr opened, DialogPtr *to, DialogClose closeHow)
{
    dlg = opened;
    global = to;
    how = closeHow;
    if (global != nullptr)
        *global = dlg;
    return dlg;
}

DialogPtr Dialog::Open(short dlogID, DialogUpdateProc update, DialogEventProc filter, DialogPtr *to,
                       DialogClose closeHow)
{
    return Opened(GetNewDialog(dlogID, update, filter), to, closeHow);
}

DialogPtr Dialog::Open(short width, short height, const DialogItemSpec *items, short count,
                       DialogUpdateProc update, DialogEventProc filter, DialogPtr *to)
{
    return Opened(NewDialogFromItems(width, height, items, count, update, filter), to, DialogClose::Plain);
}

void Dialog::Close()
{
    if (dlg == nullptr)
        return;
    if (how == DialogClose::NoFlash)
        DisposeDialogNoFlash(dlg);
    else
        DisposeDialog(dlg);
    dlg = nullptr;
    if (global != nullptr)
        *global = NULL;
}

Handle Dialog::Item(short item) const
{
    Handle h;

    GetDialogItem(dlg, item, NULL, &h, NULL);
    return h;
}

short Dialog::Modal()
{
    short hit = 0;

    ModalDialog(NULL, &hit);
    return hit;
}
