/* fontraster.h -- glyphs out of font files (port/fontraster.cpp, on FreeType). Plain C types only: the file
   that sees FreeType's headers includes no game header. */
#ifndef FONTRASTER_H
#define FONTRASTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RasterFont RasterFont;       /* one font file, opened */

/* a style the font file does not have, made up as GDI makes it up; and whether edges are smoothed */
#define kRasterBold     1                   /* the glyph smeared a pixel to the right, a pixel wider */
#define kRasterItalic   2                   /* the outline slanted */
#define kRasterSmooth   4                   /* grey edges; without it a pixel is on or off */
#define kRasterNoStrike 8                   /* the outline even where the font has a bitmap strike */

/* what a text call draws with */
typedef struct FontSpec {
    RasterFont *font;
    int pixelSize;                          /* the em, in pixels */
    int flags;                              /* kRaster* */
} FontSpec;

/* one glyph, kept by the font until it is closed */
typedef struct RasterGlyph {
    int advance;                            /* pixels to the next glyph's origin */
    int left;                               /* the bitmap's left edge, from the origin */
    int top;                                /* its top edge, above the baseline */
    int width;
    int height;
    const unsigned char *coverage;          /* width * height bytes, 0 = clear .. 255 = full */
} RasterGlyph;

/* The bytes of a font file; the font takes them over and frees them when it is closed, or at once when
   they are no font (NULL). A file of several fonts (.ttc) opens as the one of that family name, as its
   first when there is none or `family' is NULL. */
extern RasterFont *RasterOpen(void *data, int size, const char *family);
extern void RasterClose(RasterFont *font);
extern const char *RasterFamily(const RasterFont *font);    /* "Geneva" */
extern int RasterIsBold(const RasterFont *font);            /* the file itself is a bold */
extern int RasterIsItalic(const RasterFont *font);

/* the height above and below the baseline that GDI gives a font of this size (tmAscent, tmDescent) */
extern void RasterMetrics(RasterFont *font, int pixelSize, int *ascent, int *descent);

/* `unicode' is looked up in the font's Unicode character map; a font that has only a Mac Roman map is asked
   for `macRoman' (0 = it has no Mac Roman code). NULL = no glyph could be made: out of memory, or the font
   could not be set to that size, load or draw it. */
extern const RasterGlyph *RasterGetGlyph(RasterFont *font, int pixelSize, int flags, unsigned unicode,
                                         unsigned macRoman);

#ifdef __cplusplus
}
#endif
#endif
