/* gamedialog.h -- base of a game dialog run as a ModalDialog loop */
#pragma once
#include "dialog.h"
#include "dialogs.h"

/* Holds the dialog and the loop's done flag. Loop passes each item hit to Hit, which sets done to end it;
   once gQuit is set the loop ends after one more hit. */
class GameDialog {
protected:
    GameDialog() = default;
    ~GameDialog() = default;
    GameDialog(const GameDialog &) = delete;
    GameDialog &operator=(const GameDialog &) = delete;

    virtual void Hit(short item) = 0;

    void Loop()
    {
        do {
            if (gQuit)
                done = 1;
            short item = dialog.Modal();
            Hit(item);
        } while (done == 0);
    }

    Dialog dialog;
    Boolean done = 0;
};
