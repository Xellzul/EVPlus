/* prefsjson.cpp -- the preferences file as JSON: a PrefsRecord's fields by name, the key bindings as an array of
   scan codes (keycodes.h). Fields missing from the file, or not whole numbers, keep the record's value;
   uiScale is any number. */
#include "yyjson.h"
#include "burger.h"
#include "player.h"

#define kPrefsKeyCount      34          /* the bindings in use; the record holds 36 slots */

typedef struct PrefsJsonField {
    const char *name;
    size_t offset;
} PrefsJsonField;

static const PrefsJsonField kPrefsJsonFields[] = {
    { "music",          offsetof(PrefsRecord, introMusic) },
    { "voices",         offsetof(PrefsRecord, voicesOn) },
    { "noMovies",       offsetof(PrefsRecord, noMovies) },
    { "noDebris",       offsetof(PrefsRecord, noDebris) },
    { "shipAnimations", offsetof(PrefsRecord, shipAnimations) },
    { "engineGlows",    offsetof(PrefsRecord, engineGlows) },
    { "weaponGlows",    offsetof(PrefsRecord, weaponGlows) },
    { "runningLights",  offsetof(PrefsRecord, runningLights) },
    { "parallaxStars",  offsetof(PrefsRecord, parallaxStars) },
    { "ambientSounds",  offsetof(PrefsRecord, ambientSounds) },
    { "mapBorders",     offsetof(PrefsRecord, showMapBorders) },
    { "noHyperGamma",   offsetof(PrefsRecord, noHyperGamma) },
    { "yieldEvents",    offsetof(PrefsRecord, yieldEvents) },
    { "volume",         offsetof(PrefsRecord, volume) },
    { "gamma",          offsetof(PrefsRecord, gamma) },
    { "gameWidth",      offsetof(PrefsRecord, gameWidth) },
    { "gameHeight",     offsetof(PrefsRecord, gameHeight) },
    { "scaleFilter",    offsetof(PrefsRecord, scaleFilter) },
    { "wholePixelScaling", offsetof(PrefsRecord, wholePixelScaling) },
    { "stereoPan",      offsetof(PrefsRecord, stereoPan) },
    { "asteroidScaling", offsetof(PrefsRecord, asteroidScaling) },
};
#define kPrefsJsonFieldCount (sizeof(kPrefsJsonFields) / sizeof(kPrefsJsonFields[0]))

static short *FieldOf(PrefsRecord *rec, size_t i)
{
    return (short *)((char *)rec + kPrefsJsonFields[i].offset);
}

static void ReadShort(yyjson_val *v, short *out)
{
    int64_t n;

    if (!yyjson_is_int(v))
        return;
    n = yyjson_get_sint(v);
    if (n >= -32768 && n <= 32767)
        *out = (short)n;
}

void PrefsJsonLoad(const char *path, PrefsRecord *rec)
{
    char *text;
    uint32_t length;
    yyjson_doc *doc;
    yyjson_val *root;
    yyjson_val *keys;
    size_t i;

    text = (char *)LoadAFile(path, &length);
    if (text == NULL)
        return;
    doc = yyjson_read_opts(text, length, YYJSON_READ_ALLOW_BOM, NULL, NULL);
    DeallocAPointer(text);
    if (doc == NULL)
        return;
    root = yyjson_doc_get_root(doc);
    if (yyjson_is_obj(root)) {
        for (i = 0; i < kPrefsJsonFieldCount; i++)
            ReadShort(yyjson_obj_get(root, kPrefsJsonFields[i].name), FieldOf(rec, i));
        if (yyjson_is_num(yyjson_obj_get(root, "uiScale")))
            rec->uiScale = yyjson_get_num(yyjson_obj_get(root, "uiScale"));
        keys = yyjson_obj_get(root, "keys");
        for (i = 0; i < kPrefsKeyCount; i++)
            ReadShort(yyjson_arr_get(keys, i), &rec->keys[i]);
    }
    yyjson_doc_free(doc);
}

void PrefsJsonSave(const char *path, PrefsRecord *rec)
{
    yyjson_mut_doc *doc;
    yyjson_mut_val *root;
    yyjson_mut_val *keys;
    char *text;
    size_t length;
    size_t i;

    doc = yyjson_mut_doc_new(NULL);
    if (doc == NULL)
        return;
    root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    for (i = 0; i < kPrefsJsonFieldCount; i++)
        yyjson_mut_obj_add_int(doc, root, kPrefsJsonFields[i].name, *FieldOf(rec, i));
    yyjson_mut_obj_add_real(doc, root, "uiScale", rec->uiScale);
    keys = yyjson_mut_arr_with_sint16(doc, rec->keys, kPrefsKeyCount);
    yyjson_mut_obj_add_val(doc, root, "keys", keys);
    text = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_NEWLINE_AT_END, &length);
    if (text != NULL) {
        SaveAFile(path, text, (uint32_t)length);
        free(text);
    }
    yyjson_mut_doc_free(doc);
}
