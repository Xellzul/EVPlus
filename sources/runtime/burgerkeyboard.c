// COMPILE: -lang c++ -bool off
/* burgerkeyboard.c -- BurgerLib InKeyboard: the polling-proc list, the key-down table, the queue readers. Notes: Research/notes/burgerkeyboard.md */

#include "burgertypes.h"
#include "burger.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One entry of the list KeyboardKbhit runs on every poll; the list is of handles. */
typedef struct PollProcs_t {
    struct PollProcs_t **Next;                   /* +0x00 */
    void (*Proc)(void *);                        /* +0x04 */
    void *Data;                                  /* +0x08 */
} PollProcs_t;

unsigned char KeyArray[kScanCount];             /* 0x0086e098 */

static Word PollingRecurse;                      /* 0x00867480 */
static PollProcs_t **MasterKbHitProcList;        /* 0x00867484 */

#define kKeyDownBit   1

/* KeyboardCallPollingProcs (BurgerLib InKeyboard). */
// FUNCTION: EVNOVA 0x004f1800
void KeyboardCallPollingProcs(void)
{
    PollProcs_t **handle;
    PollProcs_t **next;
    PollProcs_t *entry;

    if (PollingRecurse == 0) {
        PollingRecurse = 1;
        handle = MasterKbHitProcList;
        if (handle != 0) {
            do {
                entry = *handle;
                next = entry->Next;
                entry->Proc(entry->Data);
                handle = next;
            } while (handle != 0);
        }
        PollingRecurse = 0;
    }
}

/* KeyIsDown (BurgerLib KeyboardIsPressed). */
// FUNCTION: EVNOVA 0x004f1900
unsigned int KeyIsDown(unsigned int scanCode)
{
    if (scanCode >= kScanCount)
        return 0;
    return KeyArray[scanCode] & kKeyDownBit;
}

/* KeyboardGet (BurgerLib InKeyboard). */
// FUNCTION: EVNOVA 0x004f1920
int KeyboardGet(void)
{
    if (KeyboardKbhit() == 0)
        return 0;
    return KeyboardGetch();
}

/* Drain whatever is queued, then forget every key that was down. */
/* KeyboardFlush (BurgerLib KeyboardFlush). */
// FUNCTION: EVNOVA 0x004f1940
void KeyboardFlush(void)
{
    while (KeyboardKbhit() != 0)
        KeyboardGetch();
    memset(KeyArray, 0, sizeof(KeyArray));
    MouseClicked = 0;
}

#ifdef __cplusplus
}
#endif
