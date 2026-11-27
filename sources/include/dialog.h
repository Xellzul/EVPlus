/* dialog.h -- a modal dialog and the PICTs it draws, both released when they go out of scope; the copy that ends
   an update proc; the items of a dialog built in code */
#pragma once
#include <array>
#include "macport.h"

/* A PICT kept in the global an update proc draws from; released and cleared when destroyed. */
class Picture {
public:
    Picture() = default;
    explicit Picture(PicHandle &slot) : slot(&slot) {}
    ~Picture() { Release(); }
    Picture(const Picture &) = delete;
    Picture &operator=(const Picture &) = delete;

    void Bind(PicHandle &to) { slot = &to; }
    PicHandle Load(short id) { return *slot = GetPicture(id); }
    void Release();

private:
    PicHandle *slot = nullptr;
};

/* The end of an update proc: what was drawn on the load page copied onto the dialog (whole port or one box). */
void LoadPageToDialog(DialogPtr dlg);
void LoadPageToDialog(DialogPtr dlg, const Rect &box);

/* the items of a dialog built in code, numbered from 1 as they are added */
class ItemList {
public:
    short add(unsigned char kind, short top, short left, short bottom, short right, const char *text = nullptr,
              short resID = 0)
    {
        items[count] = { kind, top, left, bottom, right, text, resID };
        return ++count;
    }
    const DialogItemSpec *data() const { return items.data(); }
    short size() const { return count; }

private:
    std::array<DialogItemSpec, 64> items;
    short count = 0;
};

/* A dialog from its DLOG/DITL (or an item list). Open sets `global` if given; Close disposes the dialog once
   and clears it. Dialogs are a stack: close them in reverse order of opening. Any idle may run the update proc
   while the dialog is open, so what it draws from (pictures, edit records, lists) is freed after Close. */
class Dialog {
public:
    Dialog() = default;
    ~Dialog() { Close(); }
    Dialog(const Dialog &) = delete;
    Dialog &operator=(const Dialog &) = delete;

    DialogPtr Open(short dlogID, DialogUpdateProc update = nullptr, DialogEventProc filter = nullptr,
                   DialogPtr *to = nullptr);
    DialogPtr Open(short width, short height, const DialogItemSpec *items, short count, DialogUpdateProc update,
                   DialogEventProc filter, DialogPtr *to = nullptr);
    void Close();

    bool IsOpen() const { return dlg != nullptr; }
    Handle Item(short item) const;
    short Modal();                  /* the item hit */

private:
    DialogPtr Opened(DialogPtr opened, DialogPtr *to);

    DialogPtr dlg = nullptr;
    DialogPtr *global = nullptr;
};
