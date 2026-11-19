/* fontraster.cpp -- glyphs out of font files, on FreeType: what GDI did for the port's text (CreateFontA,
   DrawText). Sizes, metrics and made-up styles follow GDI so that text keeps its place:
   - the size is the em in pixels (a negative lfHeight);
   - ascent and descent come from the font's VDMX table when it lists the size, else from the OS/2 table's
     Windows ascent and descent, rounded;
   - a bold the file does not have is the glyph drawn twice a pixel apart, an italic a slanted outline.
   Built with the library's enum sizes; no game header in here. */

#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H
#include "fontraster.h"

#define kBuckets        251             /* glyph cache: a small prime */
#define kItalicSlant    0x5700          /* 16.16: 0.34, about 19 degrees; not measured against GDI's */
#define kFixedOne       0x10000
#define kFrac           6               /* FreeType's 26.6 */
#define kFracHalf       32
#define kFull           255

typedef struct CachedGlyph {
    struct CachedGlyph *next;
    unsigned code;                      /* what the font's character map was asked for */
    int pixelSize;
    int flags;
    RasterGlyph glyph;                  /* coverage follows the record */
} CachedGlyph;

struct RasterFont {
    FT_Face face;
    void *data;
    FT_Byte *vdmx;                      /* the VDMX table, or NULL */
    FT_ULong vdmxSize;
    int macRomanOnly;                   /* no Unicode character map: asked by Mac Roman code */
    int setSize;                        /* the size the face is set to now */
    CachedGlyph *buckets[kBuckets];
};

static FT_Library gLibrary;

/* two names alike, upper and lower case ASCII letters taken as one */
static int SameName(const char *a, const char *b)
{
    int ca;
    int cb;

    do {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z')
            ca -= 'a' - 'A';
        if (cb >= 'a' && cb <= 'z')
            cb -= 'a' - 'A';
    } while (ca == cb && ca != 0);
    return ca == cb;
}

RasterFont *RasterOpen(void *data, int size, const char *family)
{
    RasterFont *font;
    FT_Face member;
    FT_ULong length;
    FT_Long count;
    FT_Long i;

    if (data == NULL)
        return NULL;
    if (gLibrary == NULL && FT_Init_FreeType(&gLibrary) != 0) {
        free(data);
        return NULL;
    }
    font = (RasterFont *)calloc(1, sizeof *font);
    if (font == NULL || FT_New_Memory_Face(gLibrary, (const FT_Byte *)data, size, 0, &font->face) != 0) {
        free(font);
        free(data);
        return NULL;
    }
    font->data = data;
    /* a file of several fonts (.ttc): the one of that name, where the first is another */
    count = font->face->num_faces;
    if (family != NULL && count > 1 && !SameName(RasterFamily(font), family)) {
        for (i = 1; i < count; i++) {
            if (FT_New_Memory_Face(gLibrary, (const FT_Byte *)data, size, i, &member) != 0)
                continue;
            if (member->family_name != NULL && SameName(member->family_name, family)) {
                FT_Done_Face(font->face);
                font->face = member;
                break;
            }
            FT_Done_Face(member);
        }
    }
    if (FT_Select_Charmap(font->face, FT_ENCODING_UNICODE) != 0) {
        font->macRomanOnly = 1;
        FT_Select_Charmap(font->face, FT_ENCODING_APPLE_ROMAN);
    }
    length = 0;
    if (FT_Load_Sfnt_Table(font->face, TTAG_VDMX, 0, NULL, &length) == 0 && length != 0) {
        font->vdmx = (FT_Byte *)malloc(length);
        if (font->vdmx != NULL && FT_Load_Sfnt_Table(font->face, TTAG_VDMX, 0, font->vdmx, &length) == 0) {
            font->vdmxSize = length;
        } else {
            free(font->vdmx);
            font->vdmx = NULL;
        }
    }
    return font;
}

void RasterClose(RasterFont *font)
{
    CachedGlyph *g;
    CachedGlyph *next;
    int i;

    if (font == NULL)
        return;
    for (i = 0; i < kBuckets; i++) {
        for (g = font->buckets[i]; g != NULL; g = next) {
            next = g->next;
            free(g);
        }
    }
    FT_Done_Face(font->face);
    free(font->vdmx);
    free(font->data);
    free(font);
}

const char *RasterFamily(const RasterFont *font)
{
    return font->face->family_name != NULL ? font->face->family_name : "";
}

int RasterIsBold(const RasterFont *font)
{
    return (font->face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
}

int RasterIsItalic(const RasterFont *font)
{
    return (font->face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
}

/* 0 = the font cannot be had at that size (one of bitmaps only, without that size) */
static int SetSize(RasterFont *font, int pixelSize)
{
    if (font->setSize != pixelSize) {
        font->setSize = 0;
        if (FT_Set_Pixel_Sizes(font->face, 0, pixelSize) != 0)
            return 0;
        font->setSize = pixelSize;
    }
    return 1;
}

static unsigned Be16(const FT_Byte *p)
{
    return (unsigned)p[0] << 8 | p[1];
}

/* The VDMX table: the font maker's own top and bottom for each pixel size, so that scaling and rounding
   need not be guessed. Records for square pixels only. 0 = the size is not listed. */
#define kVdmxHeader     6               /* version, numRecs, numRatios */
#define kVdmxRatio      4               /* charset, xRatio, yStartRatio, yEndRatio */
#define kVdmxGroupHead  4               /* recs, startsz, endsz */
#define kVdmxEntry      6               /* yPelHeight, yMax, yMin */

static int VdmxLookup(const RasterFont *font, int pixelSize, int *ascent, int *descent)
{
    const FT_Byte *t;
    unsigned ratios;
    unsigned offset;
    unsigned count;
    unsigned i;
    unsigned xRatio;
    unsigned yStart;
    unsigned yEnd;

    t = font->vdmx;
    if (t == NULL || font->vdmxSize < kVdmxHeader)
        return 0;
    ratios = Be16(t + 4);
    if (font->vdmxSize < kVdmxHeader + ratios * (kVdmxRatio + 2))
        return 0;
    for (i = 0; i < ratios; i++) {
        xRatio = t[kVdmxHeader + i * kVdmxRatio + 1];
        yStart = t[kVdmxHeader + i * kVdmxRatio + 2];
        yEnd = t[kVdmxHeader + i * kVdmxRatio + 3];
        if ((xRatio == 0 && yStart == 0 && yEnd == 0) || (xRatio == 1 && yStart <= 1 && yEnd >= 1))
            break;
    }
    if (i == ratios)
        return 0;
    offset = Be16(t + kVdmxHeader + ratios * kVdmxRatio + i * 2);
    if (offset + kVdmxGroupHead > font->vdmxSize)
        return 0;
    count = Be16(t + offset);
    if (offset + kVdmxGroupHead + count * kVdmxEntry > font->vdmxSize)
        return 0;
    for (i = 0; i < count; i++) {
        const FT_Byte *e = t + offset + kVdmxGroupHead + i * kVdmxEntry;

        if ((int)Be16(e) == pixelSize) {
            *ascent = (short)Be16(e + 2);
            *descent = -(short)Be16(e + 4);
            return 1;
        }
    }
    return 0;
}

void RasterMetrics(RasterFont *font, int pixelSize, int *ascent, int *descent)
{
    TT_OS2 *os2;
    FT_Fixed scale;
    int up;
    int down;

    if (VdmxLookup(font, pixelSize, ascent, descent))
        return;
    if (!SetSize(font, pixelSize)) {
        *ascent = pixelSize;
        *descent = 0;
        return;
    }
    scale = font->face->size->metrics.y_scale;
    os2 = (TT_OS2 *)FT_Get_Sfnt_Table(font->face, FT_SFNT_OS2);
    if (os2 != NULL && (os2->usWinAscent != 0 || os2->usWinDescent != 0)) {
        up = os2->usWinAscent;
        down = os2->usWinDescent;
    } else {
        up = font->face->ascender;
        down = -font->face->descender;
    }
    *ascent = (int)((FT_MulFix(up, scale) + kFracHalf) >> kFrac);
    *descent = (int)((FT_MulFix(down, scale) + kFracHalf) >> kFrac);
}

static CachedGlyph *MakeGlyph(RasterFont *font, int pixelSize, int flags, unsigned code)
{
    FT_GlyphSlot slot;
    FT_Matrix slant;
    FT_Int32 load;
    FT_UInt index;
    CachedGlyph *g;
    unsigned char *out;
    const unsigned char *row;
    int width;
    int height;
    int extra;
    int x;
    int y;
    int v;

    if (!SetSize(font, pixelSize))
        return NULL;
    index = FT_Get_Char_Index(font->face, code);
    load = (flags & kRasterSmooth) != 0 ? FT_LOAD_DEFAULT : FT_LOAD_TARGET_MONO;
    if ((flags & kRasterItalic) != 0)
        load |= FT_LOAD_NO_BITMAP;      /* a bitmap strike cannot be slanted */
    if (FT_Load_Glyph(font->face, index, load) != 0 && FT_Load_Glyph(font->face, 0, load) != 0)
        return NULL;
    slot = font->face->glyph;
    /* an embedded bitmap that is neither one bit nor one byte a pixel (2 or 4 bits, colour): the outline
       instead */
    if (slot->format == FT_GLYPH_FORMAT_BITMAP && slot->bitmap.pixel_mode != FT_PIXEL_MODE_MONO
            && slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY
            && FT_Load_Glyph(font->face, index, load | FT_LOAD_NO_BITMAP) != 0)
        return NULL;
    if ((flags & kRasterItalic) != 0 && slot->format == FT_GLYPH_FORMAT_OUTLINE) {
        slant.xx = kFixedOne;
        slant.xy = kItalicSlant;
        slant.yx = 0;
        slant.yy = kFixedOne;
        FT_Outline_Transform(&slot->outline, &slant);
    }
    if (slot->format != FT_GLYPH_FORMAT_BITMAP
            && FT_Render_Glyph(slot, (flags & kRasterSmooth) != 0 ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO) != 0)
        return NULL;
    if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_MONO && slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
        return NULL;

    width = (int)slot->bitmap.width;
    height = (int)slot->bitmap.rows;
    extra = ((flags & kRasterBold) != 0 && width != 0) ? 1 : 0;
    g = (CachedGlyph *)calloc(1, sizeof *g + (size_t)(width + extra) * height);
    if (g == NULL)
        return NULL;
    out = (unsigned char *)(g + 1);
    for (y = 0; y < height; y++) {
        row = slot->bitmap.buffer + y * slot->bitmap.pitch;
        for (x = 0; x < width; x++) {
            if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
                v = (row[x >> 3] & (0x80 >> (x & 7))) != 0 ? kFull : 0;
            else
                v = row[x];
            out[y * (width + extra) + x] = (unsigned char)v;
        }
        if (extra != 0) {
            /* the second strike, a pixel to the right, over the first */
            for (x = width; x > 0; x--) {
                if (out[y * (width + 1) + x - 1] > out[y * (width + 1) + x])
                    out[y * (width + 1) + x] = out[y * (width + 1) + x - 1];
            }
        }
    }
    g->code = code;
    g->pixelSize = pixelSize;
    g->flags = flags;
    g->glyph.advance = (int)((slot->advance.x + kFracHalf) >> kFrac) + ((flags & kRasterBold) != 0 ? 1 : 0);
    g->glyph.left = slot->bitmap_left;
    g->glyph.top = slot->bitmap_top;
    g->glyph.width = width + extra;
    g->glyph.height = height;
    g->glyph.coverage = out;
    return g;
}

const RasterGlyph *RasterGetGlyph(RasterFont *font, int pixelSize, int flags, unsigned unicode, unsigned macRoman)
{
    CachedGlyph *g;
    unsigned bucket;
    unsigned code;

    /* a font with only a Mac Roman map is asked by Mac Roman code: that code is the glyph's key */
    code = font->macRomanOnly && macRoman != 0 ? macRoman : unicode;
    bucket = (code * 31u + (unsigned)pixelSize * 7u + (unsigned)flags) % kBuckets;
    for (g = font->buckets[bucket]; g != NULL; g = g->next) {
        if (g->code == code && g->pixelSize == pixelSize && g->flags == flags)
            return &g->glyph;
    }
    g = MakeGlyph(font, pixelSize, flags, code);
    if (g == NULL)
        return NULL;
    g->next = font->buckets[bucket];
    font->buckets[bucket] = g;
    return &g->glyph;
}
