/* missionlist.h -- the mission list and briefing box of the mission computer and mission info dialogs */
#pragma once
#include "macport.h"

/* One column of rows drawn by MissionListDefinition, kept in the global handle it is built on. Reads and sets that
   handle on every call, so build one where it is used. */
class MissionList {
public:
    explicit MissionList(ScrollRec **&list) : list(list) {}

    static bool Fits(const Rect &box, short rows);  /* the rows fit in box without a scroll bar */

    void Create(DialogPtr dlg, const Rect &box, short rows, bool scrollBar);  /* rows as wide as box; locked */
    void SelectFirst() const;               /* select row 0, turn drawing on, scroll to it */
    void Leave(short row, short dir) const; /* deselect; scroll by dir if row is on the edge dir moves to */
    void Pick(short row) const;             /* select, scroll it into view */
    short Selected() const;                 /* the selected row; -1 if none, as after a click below the last row */
    void Present(DialogPtr dlg) const;      /* end of an update proc: list into loadPort, that to dlg, list on dlg */
    void Dispose();                         /* if any: dispose it and clear the handle */

private:
    ScrollRec **&list;
};

/* The briefing box (item 4) of either dialog, if visible: briefingTextPtr inverted when a mission is selected,
   else black. */
void DrawMissionBriefing(DialogPtr dlg, bool selected);
