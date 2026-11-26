/* dialog.cpp -- Dialog, Picture and LoadPageToDialog */
#include "dialog.h"
#include "dialogs.h"
#include "painter.h"

void Picture::Release()
{
    if (slot != nullptr && *slot != NULL) {
        QDReleaseResource(*slot);
        *slot = NULL;
    }
}

void LoadPageToDialog(DialogPtr dlg)
{
    LoadPageToDialog(dlg, *GetDialogPortRect(dlg));
}

void LoadPageToDialog(DialogPtr dlg, const Rect &box)
{
    UI::Painter p(UI::Surface::Dialog(dlg));

    p.Copy(UI::Surface::Load(), box, box);
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
