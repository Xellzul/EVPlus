/* glyphatlas.cpp -- the glyph atlas (glyphatlas.h): pages filled in rows, a glyph found again by its record. */
#include <unordered_map>
#include <vector>
#include "canvas.h"
#include "glyphatlas.h"

namespace UI {

namespace GlyphAtlas {

namespace {

constexpr int kPageSize = 1024;
constexpr int kGap = 1;                 /* clear pixels around each glyph: a filtered draw meets no neighbour */

/* a page and where its next glyph goes: rows from the top, each as high as its tallest glyph */
struct Page {
    PlatImage *image;
    int size;
    int x;
    int y;
    int rowHeight;
};

std::vector<Page> pages;
std::unordered_map<const RasterGlyph *, Place> places;      /* a font keeps its glyph records while the game runs */
std::vector<uint32_t> texels;
int pageSize;                           /* a page's side, once the renderer's largest texture is known */

int PageSize()
{
    if (pageSize == 0) {
        int largest = PlatMaxTextureSize();
        if (largest <= 0)
            return kPageSize;
        pageSize = largest < kPageSize ? largest : kPageSize;
    }
    return pageSize;
}

/* room for width x height on the page, a gap on every side; false when it is full */
bool Reserve(Page &page, int width, int height, int *x, int *y)
{
    if (page.x + width + kGap > page.size) {
        page.x = kGap;
        page.y += page.rowHeight + kGap;
        page.rowHeight = 0;
    }
    if (page.x + width + kGap > page.size || page.y + height + kGap > page.size)
        return false;
    *x = page.x;
    *y = page.y;
    page.x += width + kGap;
    if (height > page.rowHeight)
        page.rowHeight = height;
    return true;
}

bool NewPage()
{
    Page page = { NULL, PageSize(), kGap, kGap, 0 };

    page.image = PlatImageCreate32(page.size, page.size, NULL, 0);
    if (page.image == NULL)
        return false;
    pages.push_back(page);
    return true;
}

} // namespace

bool Find(const RasterGlyph &glyph, Place *place)
{
    PlatRect rect = { 0, 0, glyph.width, glyph.height };

    auto found = places.find(&glyph);
    if (found != places.end()) {
        *place = found->second;
        return true;
    }
    if (glyph.width <= 0 || glyph.height <= 0 || glyph.coverage == NULL)
        return false;
    if (glyph.width + 2 * kGap > PageSize() || glyph.height + 2 * kGap > PageSize())
        return false;
    if (pages.empty() || !Reserve(pages.back(), glyph.width, glyph.height, &rect.x, &rect.y)) {
        if (!NewPage() || !Reserve(pages.back(), glyph.width, glyph.height, &rect.x, &rect.y))
            return false;
    }
    texels.resize((size_t)glyph.width * glyph.height);
    for (size_t i = 0; i < texels.size(); i++)
        texels[i] = glyph.coverage[i] * 0x01010101u;
    PlatImageUpdate32(pages.back().image, &rect, texels.data(), glyph.width * (int)sizeof(uint32_t));
    place->page = pages.back().image;
    place->x = rect.x;
    place->y = rect.y;
    places[&glyph] = *place;
    return true;
}

void Clear()
{
    Canvas::FlushAll();
    for (Page &page : pages)
        PlatImageDestroy(page.image);
    pages.clear();
    places.clear();
}

} // namespace GlyphAtlas

} // namespace UI
