/* dialog.h -- a modal dialog and the PICTs it draws, both released when they go out of scope */
#pragma once
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

enum class DialogClose : unsigned char { Plain, NoFlash };

/* A dialog from its DLOG/DITL (or an item list). Open sets `global` if given; Close disposes the dialog once
   and clears it. Dialogs are a stack: close them in reverse order of opening. */
class Dialog {
public:
    Dialog() = default;
    ~Dialog() { Close(); }
    Dialog(const Dialog &) = delete;
    Dialog &operator=(const Dialog &) = delete;

    DialogPtr Open(short dlogID, DialogUpdateProc update = nullptr, DialogEventProc filter = nullptr,
                   DialogPtr *to = nullptr, DialogClose closeHow = DialogClose::Plain);
    DialogPtr Open(short width, short height, const DialogItemSpec *items, short count, DialogUpdateProc update,
                   DialogEventProc filter, DialogPtr *to = nullptr);
    void Close();

    bool IsOpen() const { return dlg != nullptr; }
    Handle Item(short item) const;
    short Modal();                  /* the item hit */

private:
    DialogPtr Opened(DialogPtr opened, DialogPtr *to, DialogClose closeHow);

    DialogPtr dlg = nullptr;
    DialogPtr *global = nullptr;
    DialogClose how = DialogClose::Plain;
};
