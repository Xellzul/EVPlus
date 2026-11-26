/* scratch.h -- a scratch vector kept between uses gives its memory back once it grew large */
#pragma once

#include <stddef.h>
#include <vector>

constexpr size_t kScratchKept = 512 * 512;      /* elements kept for the next use */

template <class T>
void SpareScratch(std::vector<T> &scratch)
{
    if (scratch.capacity() > kScratchKept)
        std::vector<T>().swap(scratch);
}
