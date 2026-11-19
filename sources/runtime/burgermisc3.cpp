/* burgermisc3.cpp -- burgermisc, exe run 3. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: EVNOVA 0x004ff830
void LinkedListInit(ListRec *list)
{
    list->head = 0;
    list->tail = 0;
    list->count = 0;
}

// FUNCTION: EVNOVA 0x004ff850
void *LinkedListNew(void)
{
    return AllocAPointerClear(sizeof(ListRec));
}

// FUNCTION: EVNOVA 0x004ff870
void *LinkedListGetEntryData(ListRec *list, ULong index)
{
    ListNode *p;

    if (list->count <= index)
        return 0;
    p = list->head;
    if (index != 0) {
        do {
            p = p->next;
        } while (--index != 0);
    }
    return p;
}

#ifdef __cplusplus
}
#endif

#define kListNodeSize   sizeof(ListNode)   /* 0x10 on 32-bit */

// FUNCTION: EVNOVA 0x004ff8a0
void LinkedListAddNewEntryEnd(ListRec *list, void *data)
{
    ListNode *node;
    ListNode *tail;

    node = (ListNode *)AllocAPointer(kListNodeSize);
    if (node != 0) {
        node->data = data;
        node->KillProc = 0;
        tail = list->tail;
        node->next = 0;
        node->prev = tail;
        if (tail != 0)
            tail->next = node;
        else
            list->head = node;
        list->tail = node;
        list->count = list->count + 1;
    }
}

// FUNCTION: EVNOVA 0x004ff900
void LinkedListAddNewEntryBegin(ListRec *list, void *data)
{
    ListNode *node;
    ListNode *head;

    node = (ListNode *)AllocAPointer(kListNodeSize);
    if (node != 0) {
        node->data = data;
        node->KillProc = 0;
        head = list->head;
        node->next = head;
        node->prev = 0;
        if (head != 0)
            head->prev = node;
        else
            list->tail = node;
        list->head = node;
        list->count = list->count + 1;
    }
}
