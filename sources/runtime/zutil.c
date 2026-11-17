/* zutil.c -- zlib 1.2.1 zutil.c, Copyright (C) 1995-2003 Jean-loup Gailly; zlib licence in zlib.h; altered: comments removed (the functions the exe does not link stay, unmarked), the heap through zlibcrt.c. Notes: Research/notes/zdeflate.md */

#include "zutil.h"

struct internal_state      {int dummy;};

const char * const z_errmsg[10] = {
"need dictionary",
"stream end",
"",
"file error",
"stream error",
"data error",
"insufficient memory",
"buffer error",
"incompatible version",
""};

/* not linked: dead-stripped in the exe; kept for the TU's data */
const char * ZEXPORT zlibVersion(void)
{
    return ZLIB_VERSION;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
uLong ZEXPORT zlibCompileFlags(void)
{
    uLong flags;

    flags = 0;
    switch (sizeof(uInt)) {
    case 2:     break;
    case 4:     flags += 1;     break;
    case 8:     flags += 2;     break;
    default:    flags += 3;
    }
    switch (sizeof(uLong)) {
    case 2:     break;
    case 4:     flags += 1 << 2;        break;
    case 8:     flags += 2 << 2;        break;
    default:    flags += 3 << 2;
    }
    switch (sizeof(voidpf)) {
    case 2:     break;
    case 4:     flags += 1 << 4;        break;
    case 8:     flags += 2 << 4;        break;
    default:    flags += 3 << 4;
    }
    switch (sizeof(z_off_t)) {
    case 2:     break;
    case 4:     flags += 1 << 6;        break;
    case 8:     flags += 2 << 6;        break;
    default:    flags += 3 << 6;
    }
    return flags;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
const char * ZEXPORT zError(err)
    int err;
{
    return ERR_MSG(err);
}

// FUNCTION: EVNOVA 0x005433a6
voidpf zcalloc (opaque, items, size)
    voidpf opaque;
    unsigned items;
    unsigned size;
{
    if (opaque) items += size - size;
    return zlib_malloc(items * size);
}

// FUNCTION: EVNOVA 0x00543393
void  zcfree (opaque, ptr)
    voidpf opaque;
    voidpf ptr;
{
    zlib_free(ptr);
    if (opaque) return;
}
