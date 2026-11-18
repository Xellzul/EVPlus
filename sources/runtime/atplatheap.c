/* atplatheap.c -- the platform layer's heap and memory primitives, and the handle manager: allocate, copy, release, resize, lock, append/insert/replace, the heap check. Notes: Research/notes/atplatheap.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#define LockResult unsigned char
#include "burgerplat.h"

/* The exe took these blocks from the process's heap (HeapAlloc), which answers a request for no bytes
   with a block; C's malloc need not. */
#define AtLeastOne(size)    ((size) != 0 ? (size) : 1)

// FUNCTION: EVNOVA 0x004fe22c
void *MemoryAllocateClear(ULong size)
{
    int32_t err;
    void *p;

    err = 0;
    p = 0;
    p = calloc(1, AtLeastOne(size));
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

// FUNCTION: EVNOVA 0x004fe25f
void MemoryDeallocate(void *p)
{
    int32_t err;

    err = 0;
    free(p);
}

// FUNCTION: EVNOVA 0x004fe2b0
void MemoryClear(void *p, ULong n)
{
    int32_t err;

    err = 0;
    memset(p, 0, n);
}

// FUNCTION: EVNOVA 0x004fe31a
void MemoryCopy(void *dst, const void *src, ULong n)
{
    int32_t err;

    err = 0;
    if (n != 0)
        memmove(dst, src, n);
}

#define kRecordsPerBlock  0x100

static MemBlock *gMemBlockList;                   /* 0x008676f0 block list */

/* Add a block of `count` free records (256 when 0) to the list. */
// FUNCTION: EVNOVA 0x004fe356
int32_t IndirectPreflight(ULong count)
{
    int32_t err;
    MemBlock *block;
    ULong i;

    err = 0;
    if (count == 0)
        count = kRecordsPerBlock;
    block = 0;
    i = sizeof(MemBlock) + (count - 1) * sizeof(MemRecord);
    block = malloc(i);
    if (block == 0) {
        err = kOutOfMemory;
    } else {
        MemoryClear(block, i);
        block->count = count;
        block->next = gMemBlockList;
        for (i = 0; i < count; i++) {
            block->records[i].guard = kRecordGuard;
            block->records[i].block = block;
        }
        gMemBlockList = block;
    }
    return err;
}

/* A new handle of `size` bytes (a zero size still gets one byte). */
// FUNCTION: EVNOVA 0x004fe3de
void **IndirectAllocate(ULong size)
{
    int32_t err, e;
    MemBlock *block;
    MemRecord *rec;
    MemRecord *found;
    ULong i;

    err = 0;
    found = 0;
    block = 0;
    rec = 0;
    e = _IndirectValidate(0);
    if (e != 0) {
        err = e;
        goto done;
    }
    block = gMemBlockList;
    for (;;) {
        if (block == 0) {
            e = IndirectPreflight(0);
            if (e != 0) {
                err = e;
                goto done;
            }
            block = gMemBlockList;
        }
        for (i = 0; i < block->count; i++) {
            if (block->records[i].used == 0)
                break;
        }
        if (i < block->count) {
            rec = &block->records[i];
            break;
        }
        block = block->next;
    }
    rec->ptr = malloc(size != 0 ? size : 1);
    if (rec->ptr == 0) {
        err = kOutOfMemory;
        goto done;
    }
    rec->size = size;
    rec->locked = 0;
    rec->used = 1;
    found = rec;
done:
    return (void **)found;
}



AT_EXTERN unsigned char (*gIndirectBusy)(void *p);         /* 0x008707f8, release hook */

// FUNCTION: EVNOVA 0x004fe501
void *IndirectInitialize(const void *src, ULong size)
{
    int32_t err;
    void **p;

    err = 0;
    p = 0;
    p = IndirectAllocate(size);
    if (p == 0)
        err = kOutOfMemory;
    else
        MemoryCopy(*p, src, size);
    return p;
}

// FUNCTION: EVNOVA 0x004fe546
int32_t IndirectDeallocate(void **p)
{
    int32_t err;
    int32_t e;

    err = 0;

    if (gIndirectBusy != 0)
        gIndirectBusy(p);
    if (gIndirectBusy != 0) {
        if (gIndirectBusy(p) != 0)
            return kBadArgument;
    }

    e = 0;
    if (_IndirectValidate(0) == 0) {
        free(p[0]);
        p[0] = 0;
        p[2] = 0;
        ((char *)p)[0xc] = 0;
        ((char *)p)[0xd] = 0;
    }
    /* BUG: no return here: the caller reads eax, _IndirectValidate's code on its failure and a
       zero left by the stores on success */
}

// FUNCTION: EVNOVA 0x004fe5b8
ULong IndirectGetSize(void **handle)
{
    int32_t err;
    ULong result;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    result = 0;
    rec = (MemRecord *)handle;
    block = rec->block;
    result = rec->size;
    return result;
}

// FUNCTION: EVNOVA 0x004fe64b
LockResult IndirectSetLock(void **handle, unsigned char lock)
{
    unsigned char old;
    int32_t err;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    old = 0;

    rec = (MemRecord *)handle;
    block = rec->block;
    old = rec->locked;
    rec->locked = lock;
    return old;
}

// FUNCTION: EVNOVA 0x004ff774
int32_t _IndirectValidate(void **handle)
{
    int32_t err;
    MemBlock *block;
    ULong i;
    unsigned char found;

    err = 0;
    block = 0;
    found = 0;
    for (block = gMemBlockList; block != 0; block = block->next) {
        for (i = 0; i < block->count; i++) {
            if ((void **)&block->records[i] == handle)
                found = 1;
            if (block->records[i].guard != kRecordGuard)
                err = kOSFailure;
            if (err == 0 && block->records[i].used != 0 && block->records[i].ptr == 0)
                err = kOSFailure;
        }
    }
    return err;
}
