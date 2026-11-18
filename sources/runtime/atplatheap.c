/* atplatheap.c -- the platform layer's heap and memory primitives, and the handle manager: allocate, copy, release, resize, lock, append/insert/replace, the heap check. Notes: Research/notes/atplatheap.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#define LockResult unsigned char
#include "burgerplat.h"
#include "win32.h"


extern void *gProcessHeap;                       /* atplatopen.c */

#define kHeapZeroMemory 8

// FUNCTION: EVNOVA 0x004fe1f9
void *MemoryAllocate(ULong size)
{
    int32_t err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    p = HeapAlloc(gProcessHeap, 0, size);
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

// FUNCTION: EVNOVA 0x004fe22c
void *MemoryAllocateClear(ULong size)
{
    int32_t err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    p = HeapAlloc(gProcessHeap, kHeapZeroMemory, size);
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

// FUNCTION: EVNOVA 0x004fe25f
void MemoryDeallocate(void *p)
{
    int32_t err;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    HeapFree(gProcessHeap, 0, p);
}

// FUNCTION: EVNOVA 0x004fe2b0
void MemoryClear(void *p, ULong n)
{
    int32_t err;

    err = 0;
    memset(p, 0, n);
}

/* Byte-fills up to the first dword boundary, then dwords, then the tail. */
// FUNCTION: EVNOVA 0x004fe2c3
void MemoryFill(void *p, int c, ULong n)
{
    int32_t err;
    ULong fill;
    ULong count;
    ULong i;
    unsigned char *d;
    unsigned char b;

    err = 0;
    fill = b = (ULong)(unsigned char)c;
    d = p;
    count = n;
    fill |= fill << 16;
    while (((ULong)d & 3) != 0 && count != 0) {
        *d++ = b;
        count--;
    }
    fill |= fill << 8;
    i = count >> 2;
    while (i--) {
        *(ULong *)d = fill;
        d += 4;
    }
    i = count & 3;
    while (i--)
        *d++ = b;
}

// FUNCTION: EVNOVA 0x004fe31a
void MemoryCopy(void *dst, const void *src, ULong n)
{
    int32_t err;

    err = 0;
    if (n != 0)
        memmove(dst, src, n);
}

// FUNCTION: EVNOVA 0x004fe338
void MemoryMove(void *dst, const void *src, ULong n)
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
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
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
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
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


/* IndirectAllocateTempMem (a new handle through IndirectAllocate, after the same three discarded checks;
   the FT loader's alternative allocator). */
// FUNCTION: EVNOVA 0x004fe493
void **IndirectAllocateTempMem(ULong size)
{
    int32_t err;
    void **result;

    err = 0;
    result = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    result = IndirectAllocate(size);
    return result;
}

/* A new handle holding a copy of another's bytes. */
// FUNCTION: EVNOVA 0x004fe4b5
void **IndirectDuplicate(void **handle)
{
    int32_t err;
    void **copy;
    ULong size;
    MemRecord *rec;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    rec = (MemRecord *)handle;
    size = IndirectGetSize(handle);
    copy = IndirectAllocate(size);
    if (copy == 0)
        err = kOutOfMemory;
    else
        MemoryCopy(*copy, rec->ptr, size);
    return copy;
}


AT_EXTERN unsigned char (*gIndirectBusy)(void *p);         /* 0x008707f8, release hook */

// FUNCTION: EVNOVA 0x004fe501
void *IndirectInitialize(const void *src, ULong size)
{
    int32_t err;
    void **p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
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
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();

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

// FUNCTION: EVNOVA 0x004fe5cd
int32_t IndirectSetSize(void **handle, ULong size)
{
    int32_t err;
    MemRecord *rec;
    MemBlock *block;
    void *p;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        rec = (MemRecord *)handle;
        block = rec->block;
        p = realloc(rec->ptr, size != 0 ? size : 1);
        if (p == 0) {
            err = kOutOfMemory;
        } else {
            rec->ptr = p;
            rec->size = size;
            rec->locked = 0;
            rec->used = 1;
        }
    }
    return err;
}

// FUNCTION: EVNOVA 0x004fe636
LockResult IndirectGetLock(void **handle)
{
    int32_t err;
    unsigned char result;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    result = 0;
    rec = (MemRecord *)handle;
    block = rec->block;
    result = rec->locked;
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
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();

    rec = (MemRecord *)handle;
    block = rec->block;
    old = rec->locked;
    rec->locked = lock;
    return old;
}

/* IndirectAppend (append one handle's bytes to another). */
// FUNCTION: EVNOVA 0x004fe674
int32_t IndirectAppend(void **handle, void **from)
{
    int32_t err, e;
    ULong old;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        length = IndirectGetSize(from);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else
                MemoryCopy((char *)*handle + old, *from, length);
        }
    }
    return err;
}

/* IndirectReplace (make a handle a copy of another). */
// FUNCTION: EVNOVA 0x004fe6f1
int32_t IndirectReplace(void **handle, void **from)
{
    int32_t err, e;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        length = IndirectGetSize(from);
        e = IndirectSetSize(handle, length);
        if (e != 0)
            err = e;
        else
            MemoryCopy(*handle, *from, length);
    }
    return err;
}

/* IndirectInsertData (insert bytes at the front of a handle). */
// FUNCTION: EVNOVA 0x004fe74e
int32_t IndirectInsertData(void **handle, const void *data, ULong length)
{
    int32_t err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else {
                MemoryMove((char *)*handle + length, *handle, old);
                MemoryCopy(*handle, data, length);
            }
        }
    }
    return err;
}

/* IndirectAppendData (append bytes to a handle). */
// FUNCTION: EVNOVA 0x004fe7c3
int32_t IndirectAppendData(void **handle, const void *data, ULong length)
{
    int32_t err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else
                MemoryCopy((char *)*handle + old, data, length);
        }
    }
    return err;
}

/* IndirectReplaceData (replace a handle's contents with length bytes of data). */
// FUNCTION: EVNOVA 0x004fe82c
int32_t IndirectReplaceData(void **handle, const void *data, ULong length)
{
    int32_t err, e;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        e = IndirectSetSize(handle, length);
        if (e != 0)
            err = e;
        else if (length != 0)
            MemoryCopy(*handle, data, length);
    }
    return err;
}

/* Replaces oldLength bytes at offset with newLength bytes of data. */
// FUNCTION: EVNOVA 0x004fe884
int32_t IndirectSpliceData(void **handle, ULong offset, ULong oldLength, const void *data,
                  ULong newLength)
{
    int32_t err, e;
    ULong size;
    ULong len;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    size = IndirectGetSize(handle);
    len = newLength;
    if (offset + oldLength > size) {
        err = kBadArgument;
        goto done;
    }
    if (oldLength < newLength) {
        e = IndirectSetSize(handle, size + newLength - oldLength);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    MemoryMove((char *)*handle + offset + newLength,
                 (char *)*handle + offset + oldLength, size - (offset + oldLength));
    MemoryCopy((char *)*handle + offset, data, newLength);
    if (oldLength > newLength) {
        e = IndirectSetSize(handle, size + newLength - oldLength);
        if (e != 0)
            err = e;
    }
done:
    return err;
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
        IsBadReadPtr(block, block->count * sizeof(ULong));
        IsBadWritePtr(block, block->count * sizeof(ULong));
        for (i = 0; i < block->count; i++) {
            if ((void **)&block->records[i] == handle)
                found = 1;
            if (block->records[i].guard != kRecordGuard)
                err = kOSFailure;
            if (err == 0 && block->records[i].used != 0 && block->records[i].ptr == 0)
                err = kOSFailure;
        }
    }
    if (handle != 0 && IndirectGetSize(handle) != 0) {
        IsBadReadPtr(*handle, IndirectGetSize(handle));
        IsBadWritePtr(*handle, IndirectGetSize(handle));
    }
    return err;
}
