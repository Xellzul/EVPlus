// COMPILE: -lang c++ -bool off
/* burgermemory.c -- BurgerLib's handle and pointer calls on the C library's heap. Notes: Research/notes/burgermemory.md

   The exe had BurgerLib's MMMemoryManager here: one pool taken at start (CE: 2040 MB), handle records inside it,
   compaction and purging when it ran full. The port keeps the calls and gives each handle its own malloc block:
   nothing moves unless resized, nothing is purged (resources marked purgeable stay until released), and a
   failed request fails at once instead of purging first. With the pool, never filled in practice, that is
   how it behaved anyway. Every block starts zeroed, as the fresh pool's memory did. */

#include <stdlib.h>
#include <string.h>

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A Handle is the address of one of these, so its first field is the master pointer. The record stays where
   it is for the handle's life; only the memory it points to moves (ResizeAHandle). */
typedef struct MyHandle {
    void *MemPtr;
    Word32 Length;                               /* the size asked for */
    Word32 Flags;                                /* as given; nothing reads them now */
} MyHandle;

/* AllocAPointer's block: the size in front, the caller's memory 16 bytes in (the exe's header size, which
   also keeps the memory 16-byte aligned on every system). */
#define kPointerHeader  0x10

/* AllocAHandle2 (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d82d0
void **AllocAHandle2(Word32 MemSize, Word Flag)
{
    MyHandle *h;

    if (MemSize == 0)
        return 0;
    h = (MyHandle *)calloc(1, sizeof(MyHandle));
    if (h != 0) {
        h->MemPtr = calloc(1, MemSize);
        if (h->MemPtr != 0) {
            h->Length = MemSize;
            h->Flags = Flag;
            return (void **)h;
        }
        free(h);
    }
    NonFatal("Out of memory, I need %ld bytes\n", (long)MemSize);
    return 0;
}

/* AllocAPointer (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d86d0
void *AllocAPointer(Word32 size)
{
    char *block;

    if (size == 0)
        return 0;
    block = (char *)calloc(1, kPointerHeader + (size_t)size);
    if (block == 0) {
        NonFatal("Out of memory, I need %ld bytes\n", (long)size);
        return 0;
    }
    *(Word32 *)block = size;
    return block + kPointerHeader;
}

/* DeallocAHandle (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d8710
void DeallocAHandle(void *MemHandle)
{
    MyHandle *h;

    h = (MyHandle *)MemHandle;
    if (h != 0) {
        free(h->MemPtr);
        free(h);
    }
}

/* DeallocAPointer (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d87d0
void DeallocAPointer(void *MemPtr)
{
    if (MemPtr != 0)
        free((char *)MemPtr - kPointerHeader);
}

/* ResizeAPointer (BurgerLib MMMemoryManager): AllocAPointer for no pointer, DeallocAPointer for no size; a
   failed resize frees the old memory too, as the exe's did. Memory gained is zeroed. */
// FUNCTION: EVNOVA 0x004d87f0
void *ResizeAPointer(void *mem, uint32_t size)
{
    char *block;
    char *grown;
    Word32 old;

    if (mem == 0)
        return AllocAPointer(size);
    if (size == 0) {
        DeallocAPointer(mem);
        return 0;
    }
    block = (char *)mem - kPointerHeader;
    old = *(Word32 *)block;
    grown = (char *)realloc(block, kPointerHeader + (size_t)size);
    if (grown == 0) {
        free(block);
        NonFatal("Out of memory, I need %ld bytes\n", (long)size);
        return 0;
    }
    if (size > old)
        memset(grown + kPointerHeader + old, 0, size - old);
    *(Word32 *)grown = size;
    return grown + kPointerHeader;
}

/* ResizeAHandle (BurgerLib MMMemoryManager): AllocAHandle2 for no handle, DeallocAHandle for no size; the
   handle stays the same, its memory may move. A failed resize frees the handle, as the exe's did. Memory
   gained is zeroed. */
// FUNCTION: EVNOVA 0x004d8890
Handle ResizeAHandle(Handle h, uint32_t size)
{
    MyHandle *hand;
    char *grown;

    hand = (MyHandle *)h;
    if (hand == 0)
        return (Handle)AllocAHandle2(size, 0);
    if (size == 0) {
        DeallocAHandle(hand);
        return 0;
    }
    if (size == hand->Length)
        return h;
    grown = (char *)realloc(hand->MemPtr, size);
    if (grown == 0) {
        DeallocAHandle(hand);
        NonFatal("Out of memory, I need %ld bytes\n", (long)size);
        return 0;
    }
    if (size > hand->Length)
        memset(grown + hand->Length, 0, size - hand->Length);
    hand->MemPtr = grown;
    hand->Length = size;
    return h;
}

/* GetAHandleSize (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d8ae0
uint32_t GetAHandleSize(Handle h)
{
    if (h != 0)
        return ((MyHandle *)h)->Length;
    return 0;
}

/* GetAPointerSize (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d8b00
uint32_t GetAPointerSize(void *mem)
{
    if (mem != 0)
        return *(Word32 *)((char *)mem - kPointerHeader);
    return 0;
}

/* LockAHandle (BurgerLib MMMemoryManager): nothing moves memory now, so only the master pointer. */
// FUNCTION: EVNOVA 0x004d8b80
char *LockAHandle(Handle h)
{
    if (h != 0)
        return *h;
    return 0;
}

/* UnlockAHandle (BurgerLib MMMemoryManager). */
// FUNCTION: EVNOVA 0x004d8ba0
void UnlockAHandle(Handle h)
{
    (void)h;
}

/* SetHandlePurgeFlag (BurgerLib MMMemoryManager): nothing is purged now. */
// FUNCTION: EVNOVA 0x004d8bb0
void SetHandlePurgeFlag(void **handle, Word flag)
{
    (void)handle;
    (void)flag;
}

#ifdef __cplusplus
}
#endif
