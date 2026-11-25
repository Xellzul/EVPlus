/* missionlist.cpp -- MissionList and the briefing box of the two mission dialogs */
#include "gameconst.h"
#include "dialogs.h"
#include "render.h"
#include "dialog.h"
#include "missionlist.h"

static constexpr short kRowHeight = kListItemBaseHeight * 1.5;

bool MissionList::Fits(const Rect &box, short rows)
{
    return kRowHeight * rows <= box.bottom - box.top;
}

void MissionList::Create(DialogPtr dlg, const Rect &box, short rows, bool scrollBar)
{
    Rect cellBounds;
    Point cellSize;

    SetRect(&cellBounds, 0, 0, 1, rows);
    cellSize.v = kRowHeight;
    cellSize.h = box.right - box.left;
    list = nv_NewDialog(&box, &cellBounds, cellSize, MissionListDefinition, (CGrafPtr)dlg,
                        scrollBar);
    LockAHandle((Handle)list);
}

void MissionList::SelectFirst() const
{
    Point first;

    first.h = first.v = 0;
    LSetSelect(1, first, list);
    LSetDrawingMode(1, list);
    LAutoScroll(list);
}

void MissionList::Leave(short row, short dir) const
{
    Point cell;
    Rect box;

    if (row == -1)
        return;
    cell.h = 0;
    cell.v = row;
    LSetSelect(0, cell, list);
    GetListCellRect(&box, cell, list);
    if (dir > 0 ? box.bottom >= (*list)->rView.bottom : box.top <= (*list)->rView.top)
        LScroll(0, dir, list);
}

void MissionList::Pick(short row) const
{
    Point cell;

    cell.h = 0;
    cell.v = row;
    LSetSelect(1, cell, list);
    LAutoScroll(list);
}

short MissionList::Selected() const
{
    Point cell;

    cell.h = 0;
    for (cell.v = 0; cell.v < (*list)->dataBounds.bottom; cell.v++) {
        if (LGetSelect(0, &cell, list))
            return cell.v;
    }
    return -1;
}

void MissionList::Present(DialogPtr dlg) const
{
    if (list != NULL)
        LUpdate(list);
    CopyLoadPortToDialog(dlg);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (list != NULL) {
        SetDialogGWorld(dlg);
        LUpdate(list);
    }
}

void MissionList::Dispose()
{
    if (list != NULL) {
        LDispose(list);
        list = NULL;
    }
}

void DrawMissionBriefing(DialogPtr dlg, bool selected)
{
    Rect box;

    GetDialogItem(dlg, 4, NULL, NULL, &box);
    if (selected) {
        ForeColor(blackColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
        InvertRect(&box);
    } else {
        ForeColor(blackColor);
        PaintRect(&box);
    }
}
