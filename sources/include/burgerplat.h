/* burgerplat.h -- BurgerLib's platform-layer records; types only. Notes: Research/notes/atplat.md */
#ifndef BURGERPLAT_H
#define BURGERPLAT_H

#include "burgertypes.h"

/* the doubly-linked list */
typedef struct ListNode {
    struct ListNode *next;                       /* +0x00 */
    struct ListNode *prev;                       /* +0x04 */
    void *data;                                  /* +0x08 */
    void (*KillProc)(struct ListNode *);         /* +0x0c */
} ListNode;

typedef struct ListRec {
    ListNode *head;                              /* +0x00 */
    ListNode *tail;                              /* +0x04 */
    ULong count;                                 /* +0x08 */
} ListRec;

/* BurgerLib's parsed-audio descriptor */
typedef struct AudioDesc {
    Word8                  *SoundPtr;                /* +0x00 */
    ULong                  SoundLength;              /* +0x04 */
    ULong                  LoopStart;                /* +0x08 */
    ULong                  LoopEnd;                  /* +0x0c */
    void                   (*Proc)(void *);          /* +0x10 */
    void                   *Data;                    /* +0x14 */
    ULong                  DataType;                 /* +0x18 */
    ULong                  SampleRate;               /* +0x1c */
    Word8                  *CompPtr;                 /* +0x20 */
    ULong                  Extra1;                   /* +0x24 */
    ULong                  Extra2;                   /* +0x28 */
    ULong                  Extra3;                   /* +0x2c */
    ULong                  SampleCount;              /* +0x30 */
    float                  TimeInSeconds;            /* +0x34 */
} AudioDesc;

#endif
