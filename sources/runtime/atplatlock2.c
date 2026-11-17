/* atplatlock2.c -- atplatlock, exe run 2. Notes: Research/notes/atplatlock.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

ULong gPlatLockOwnerThread;                              /* 0x008705d0 owning thread */
static int32_t gAtomicCount;                        /* 0x0086b6b8 lock depth */

// FUNCTION: EVNOVA 0x00505f03
void AtomicEnterCritical(void *token)
{
    int32_t *state;
    int32_t err;

    state = token;
    err = 0;
    if (gPlatLockOwnerThread != GetCurrentThreadId()) {
        EnterCriticalSection(gPlatCritSec);
        gPlatLockOwnerThread = GetCurrentThreadId();
    }
    gAtomicCount = gAtomicCount + 1;
}

// FUNCTION: EVNOVA 0x00505f36
void AtomicLeaveCritical(void *token)
{
    int32_t *state;
    int32_t err;

    state = token;
    err = 0;
    gAtomicCount = gAtomicCount - 1;
    if (gAtomicCount == 0) {
        GetCurrentThreadId();
        gPlatLockOwnerThread = 0;
        LeaveCriticalSection(gPlatCritSec);
    }
}

/* Atomic bit test, test-and-set and test-and-clear on a state word, each returning the
   bit's previous value. With InterlockedCompareExchange resolved and the word
   dword-aligned they spin on it; otherwise they take the recursive lock above. */

typedef int32_t (__stdcall *CompareExchangeProc)(ULong *dest, ULong exchange,
                                             ULong comparand);

extern void *gPlatCmpXchgProc;                       /* atplatopen.c */

#define kWordAlignMask 3
#define kMisaligned -7001

typedef int32_t (__stdcall *ExchangeAddProc)(int32_t *dest, int32_t delta);

extern void *gPlatExchangeAddProc;                       /* atplatopen.c */

/* AtomicIncrement (atomic add answering the old value: InterlockedExchangeAdd when resolved
   and the word aligned, else under the recursive lock). */
// FUNCTION: EVNOVA 0x00505f66
int32_t AtomicIncrement(int32_t *value, int32_t delta)
{
    char token[16];
    int32_t old;
    int32_t err;
    ULong misaligned;

    err = 0;
    old = 0;
    misaligned = (ULong)value & kWordAlignMask;
    if (misaligned != 0)
        err = kMisaligned;
    if (gPlatExchangeAddProc != 0 && misaligned == 0) {
        old = ((ExchangeAddProc)gPlatExchangeAddProc)(value, delta);
    } else {
        AtomicEnterCritical(token);
        old = *value;
        *value += delta;
        AtomicLeaveCritical(token);
    }
    return old;
}

// FUNCTION: EVNOVA 0x00505fc2
unsigned char AtomicMaskApply(ULong *word, ULong bit)
{
    char token[16];
    unsigned char old;
    ULong mask;
    ULong cur;
    int32_t err;
    ULong next;
    ULong result;

    err = 0;
    old = 1;
    if (gPlatCmpXchgProc != 0 && ((ULong)word & kWordAlignMask) == 0) {
        do {
            mask = 1 << bit;
            cur = *word;
            next = cur;
            next |= mask;
            result = ((CompareExchangeProc)gPlatCmpXchgProc)(word, next, cur);
        } while (result != cur);
        if ((cur & mask) != 0)
            old = 1;
        else
            old = 0;
    } else {
        bit = 1 << bit;
        if ((*word & bit) == 0) {
            AtomicEnterCritical(token);
            if ((*word & bit) != 0)
                old = 1;
            else
                old = 0;
            *word |= bit;
            AtomicLeaveCritical(token);
        }
    }
    return old;
}

// FUNCTION: EVNOVA 0x00506046
unsigned char AtomicMaskClear(ULong *word, ULong bit)
{
    char token[16];
    unsigned char old;
    ULong mask;
    ULong cur;
    int32_t err;
    ULong next;
    ULong result;

    err = 0;
    old = 0;
    if (gPlatCmpXchgProc != 0 && ((ULong)word & kWordAlignMask) == 0) {
        do {
            mask = 1 << bit;
            next = ~mask;
            cur = *word;
            next &= cur;
            result = ((CompareExchangeProc)gPlatCmpXchgProc)(word, next, cur);
        } while (result != cur);
        if ((cur & mask) != 0)
            old = 1;
        else
            old = 0;
    } else {
        bit = mask = 1 << bit;
        if ((*word & bit) != 0) {
            AtomicEnterCritical(token);
            if ((*word & bit) != 0)
                old = 1;
            else
                old = 0;
            *word &= ~mask;
            AtomicLeaveCritical(token);
        }
    }
    return old;
}

// FUNCTION: EVNOVA 0x005060d0
unsigned char AtomicMaskTest(ULong *word, ULong bit)
{
    int32_t err;
    unsigned char result;

    err = 0;
    result = 0;
    result = (*word & (1 << bit)) ? true : false;
    return result;
}

// FUNCTION: EVNOVA 0x005060f4
unsigned char _AtomicCheckCritical(void)
{
    unsigned char owner;
    ULong id;
    int32_t err;

    err = 0;
    owner = 0;
    id = 0;
    if (gPlatLockOwnerThread == GetCurrentThreadId())
        owner = 1;
    return owner;
}

// FUNCTION: EVNOVA 0x00506113
void _AtomicResetCritical(void)
{
    gAtomicCount = 0;
}
