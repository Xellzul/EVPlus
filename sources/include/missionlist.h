/* missionlist.h -- the mission list and briefing box of the mission computer and mission info dialogs */
#pragma once
#include "macport.h"

namespace UI {
class Painter;
class Surface;
}

/* one column of rows (MissionListDefinition) in a global handle, read on every call: build one where it is used */
class MissionList {
public:
    explicit MissionList(ScrollRec **&list) : list(list) {}

    static bool Fits(const Rect &box, short rows);  /* the rows fit in box without a scroll bar */

    void Create(DialogPtr dlg, const Rect &box, short rows, bool scrollBar);  /* rows as wide as box; locked */
    void SelectFirst() const;               /* select row 0, turn drawing on, scroll to it */
    void Leave(short row, short dir) const; /* deselect; scroll by dir if row is on the edge dir moves to */
    void Pick(short row) const;             /* select, scroll it into view */
    short Selected() const;                 /* the selected row; -1 if none, as after a click below the last row */
    /* end of an update proc: the list drawn on the load page, that copied to dlg, the list drawn on dlg */
    void Present(UI::Painter &load, DialogPtr dlg) const;
    void Dispose();                         /* if any: dispose it and clear the handle */

private:
    UI::Surface Dialog() const;

    ScrollRec **&list;
};

/* the briefing box (item 4) of either dialog: briefingTextPtr white on black for a selected mission, else black */
void DrawMissionBriefing(UI::Painter &p, DialogPtr dlg, bool selected);
