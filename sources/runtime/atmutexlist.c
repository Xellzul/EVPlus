/* atmutexlist.c -- the runtime block's mutex-guarded singly linked list, the one the thread table keeps its records on. Notes: Research/notes/atmutexlist.md */

#include "burgertypes.h"
#include "burger.h"
#ifndef MAC1010
#include "win32.h"
#endif

#include "burgerplat.h"

// FUNCTION: EVNOVA 0x0052b99b
int32_t QueueCreate(void *ref)
{
    MutexList *list;
    int32_t err;
    int32_t e;

    list = ref;
    err = 0;
    e = 0;
    SystemCheckCallback();
    MemoryClear(list, sizeof(MutexList));
    list->mutex = CreateMutexA(0, 0, 0);
    if (list->mutex == 0)
        err = kOutOfMemory;
    if (err != 0)
        QueueDispose(ref);
    return err;
}

// FUNCTION: EVNOVA 0x0052b9e4
void QueueDispose(MutexList *list)
{
    int32_t err;
    int32_t e;
    void *mutex;
    void *h;

    err = 0;
    e = 0;
    SystemCheckCallback();
    if (list->mutex != 0)
        WaitForSingleObject(list->mutex, INFINITE);
    h = list->mutex;
    if (h != 0) {
        mutex = h;
        list->mutex = 0;
        ReleaseMutex(h);
        CloseHandle(h);
    }
    MemoryClear(list, sizeof(MutexList));
}

// FUNCTION: EVNOVA 0x0052ba30
void QueueInsert(void *ref, void *element)
{
    MutexList *list;
    MutexNode *node;
    int32_t err;

    list = ref;
    node = element;
    err = 0;
    node->next = 0;
    node->owner = ref;
    WaitForSingleObject(list->mutex, INFINITE);
    if (list->tail != 0)
        list->tail->next = node;
    else
        list->head = node;
    list->tail = node;
    list->count++;
    ReleaseMutex(list->mutex);
}

// FUNCTION: EVNOVA 0x0052ba71
MutexNode *QueueRemove(MutexList *list)
{
    MutexNode *node;
    int32_t err;

    node = 0;
    err = 0;
    if (list->head != 0) {
        WaitForSingleObject(list->mutex, INFINITE);
        node = list->head;
        if (node != 0) {
            list->head = node->next;
            if (list->head == 0)
                list->tail = 0;
            node->next = 0;
            list->count--;
        }
        ReleaseMutex(list->mutex);
    }
    if (node != 0)
        node->owner = 0;
    return node;
}

// FUNCTION: EVNOVA 0x0052babf
MutexNode *QueueExtract(void *ref, void *element)
{
    MutexList *list;
    MutexNode **link;
    MutexNode *p;
    MutexNode *found;
    int32_t err;

    list = ref;
    found = element;
    err = 0;
    link = 0;
    if (list->head != 0) {
        WaitForSingleObject(list->mutex, INFINITE);
        link = &list->head;
        while ((p = *link) != 0) {
            if (p == found)
                break;
            link = &p->next;
        }
        if (p != 0) {
            *link = found->next;
            if (list->head == 0) {
                list->tail = 0;
            } else if (*link == 0) {
                for (list->tail = list->head; list->tail->next != 0; list->tail = list->tail->next)
                    ;
            }
            found->next = 0;
            list->count--;
        } else {
            found = 0;
        }
        ReleaseMutex(list->mutex);
    } else {
        found = 0;
    }
    if (found != 0)
        found->owner = 0;
    if (found != 0)
        return element;
    return 0;
}

/* QueueTransfer (move every node of one list onto another). */
// FUNCTION: EVNOVA 0x0052bb4e
void QueueTransfer(void *dstRef, void *srcRef)
{
    char token[16];
    MutexList *dst;
    MutexList *src;
    MutexNode *node;
    int32_t err;

    dst = dstRef;
    src = srcRef;
    err = 0;
    if (src->head != 0) {
        AtomicEnterCritical(token);
        while ((node = QueueRemove(srcRef)) != 0)
            QueueInsert(dstRef, node);
        AtomicLeaveCritical(token);
    }
}

/* QueueGetCount (a list's node count). */
// FUNCTION: EVNOVA 0x0052bb97
ULong QueueGetCount(void *ref)
{
    MutexList *list;
    int32_t err;
    ULong result;

    list = ref;
    err = 0;
    result = 0;
    result = list->count;
    return result;
}

/* QueueIsEmpty (a list is empty). */
// FUNCTION: EVNOVA 0x0052bbab
unsigned char QueueIsEmpty(void *ref)
{
    MutexList *list;
    int32_t err;
    unsigned char result;

    list = ref;
    err = 0;
    result = true;
    if (list->head != 0)
        result = false;
    return result;
}

/* QueueEmpty (free every node of a list). */
// FUNCTION: EVNOVA 0x0052bbbf
void QueueEmpty(MutexList *list)
{
    MutexNode *node;
    int32_t err;

    err = 0;
    node = 0;
    SystemCheckCallback();
    while ((node = QueueRemove(list)) != 0)
        MemoryDeallocate(node);
}
