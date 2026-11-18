/* atthreadtbl.c -- the platform layer's lock. The exe's file also holds the thread table (records, queues,
   create and dispose); its one user was the clock's timer thread. Notes: Research/notes/atthreadtbl.md */

#include "burgertypes.h"
#include "burger.h"
#include "platform.h"

#define kMutexMagic       0xdead1ce5UL

// FUNCTION: EVNOVA 0x005044ce
int32_t CriticalCreate(void *lock)
{
    Mutex *m;

    m = lock;
    memset(m, 0, sizeof(Mutex));
    m->magic = kMutexMagic;
    if ((m->section = PlatMutexCreate()) == 0) {
        CriticalDispose(lock);
        return kOutOfMemory;
    }
    m->live = 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0050454f
int32_t CriticalDispose(void *lock)
{
    Mutex *m;

    m = lock;
    if (m->section != 0)
        PlatMutexDestroy(m->section);
    memset(m, 0, sizeof(Mutex));
    return 0;
}

// FUNCTION: EVNOVA 0x00504592
void CriticalEnter(void *lock)
{
    Mutex *m;

    m = lock;
    PlatMutexLock(m->section);
}

// FUNCTION: EVNOVA 0x005045cc
void CriticalLeave(void *lock)
{
    Mutex *m;

    m = lock;
    PlatMutexUnlock(m->section);
}
