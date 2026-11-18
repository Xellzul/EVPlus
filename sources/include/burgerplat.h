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

/* BurgerLib's directory walker. The exe's also had the entry's two dates and whether it was hidden, a
   system file or read-only, none of them read, and Windows' own search record. */
typedef struct DirectorySearch_t {
    Word32 FileSize;                             /* 0 for an empty file; see burgerfilewin.c */
    char Dir;
    char Name[1024];                             /* UTF-8 */
    void *FindHandle;                            /* the folder's listing */
    Word HandleOk;                               /* the search is open */
} DirectorySearch_t;

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

/* a handle is a record pointer; the record's first word is the block */
struct MemBlock;

typedef struct MemRecord {
    void *ptr;                                   /* +0x00 */
    ULong guard;                                 /* +0x04 */
    ULong size;                                  /* +0x08 */
    unsigned char locked;                        /* +0x0c */
    unsigned char used;                          /* +0x0d */
    struct MemBlock *block;                      /* +0x10 */
} MemRecord;

typedef struct MemBlock {
    ULong count;                                 /* +0x00 */
    ULong reserved[2];                           /* +0x04 */
    struct MemBlock *next;                       /* +0x0c */
    MemRecord records[1];                        /* +0x10 */
} MemBlock;

#define kRecordGuard      0xdeadbeefUL

/* a text buffer that grows its handle */
typedef struct TextBuilder {
    char **handle;                               /* +0x00, a Handle */
    ULong length;                                /* +0x04 */
    ULong capacity;                             /* +0x08 */
    ULong extent;                                /* +0x0c */
    int32_t failed;                                 /* +0x10 */
} TextBuilder;

#endif
