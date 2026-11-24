/* burgerfilemgr.cpp -- whole files loaded and saved, the stream helpers. Notes: Research/notes/burgerfilemgr.md */


#include "burger.h"
#include "platform.h"

/* The exe takes the address of its own PARAMETER SLOT (`lea eax,[esp+0x14]'
   is arg1, not a local), so this writes the caller's value out. */
/* fwritelong (BurgerLib's void signature). */
// FUNCTION: EVNOVA 0x004f22b0
void fwritelong(int32_t value, FILE *stream)
{
    fwrite(&value, 1, 4, stream);
}

/* Writes the string AND its terminator -- the count is strlen + 1. */
/* fwritestr (BurgerLib's void signature). */
// FUNCTION: EVNOVA 0x004f22e0
void fwritestr(const char *s, FILE *stream)
{
    fwrite(s, 1, strlen(s) + 1, stream);
}

/* fgetlong (BurgerLib fgetlong). */
// FUNCTION: EVNOVA 0x004f2310
int32_t fgetlong(FILE *stream)
{
    int32_t value;

    fread(&value, 1, 4, stream);
    return value;
}

/* Reads to the NUL whatever the buffer holds; 1 on the NUL, 0 on EOF. */
// FUNCTION: EVNOVA 0x004f2350
int32_t fgetstr(char *buf, int32_t size, FILE *stream)
{
    char *end;
    int32_t c;

    end = buf + size;
    for (;;) {
        c = fgetc(stream);
        if (c == 0 || c == -1)
            break;
        if (buf < end - 1)
            *buf++ = c;
    }
    if (size != 0)
        *buf = 0;
    if (c != 0)
        return 0;
    return 1;
}

/* The data as the whole file; 1 when written. */
// FUNCTION: EVNOVA 0x004f23d0
int32_t SaveWholeFile(const char *path, const void *data, uint32_t length)
{
    FILE *file;

    file = PlatFileOpen(path, "wb");
    if (file == 0) {
        NonFatal("Can't open the file to save to\n");
        return 0;
    }
    return SaveAFileFP(file, data, length);
}

// FUNCTION: EVNOVA 0x004f2430
int32_t SaveAFileFP(FILE *file, const void *data, uint32_t length)
{
    uint32_t written;
    int closed;

    if (file != 0) {
        written = fwrite(data, 1, length, file);
        closed = fclose(file);
        if (written == length && closed == 0)
            return 1;
    }
    NonFatal("Error in saving file contents\n");
    return 0;
}

/* The whole file in a new pointer. */
// FUNCTION: EVNOVA 0x004f24c0
void *LoadWholeFile(const char *path, uint32_t *length)
{
    FILE *fp;

    fp = PlatFileOpen(path, "rb");
    if (fp != 0)
        return LoadAFileFP(fp, length);
    NonFatal("File was not found\n");
    if (length != 0)
        *length = 0;
    return 0;
}

/* LoadAFileFP (BurgerLib FmFileManager): the whole of an open file in a new pointer; closes
   it. */
// FUNCTION: EVNOVA 0x004f2530
void *LoadAFileFP(FILE *fp, uint32_t *length)
{
    ULong size;
    void *data;

    data = 0;
    size = 0;
    if (fp != 0) {
        size = fgetfilesize(fp);
        if (size != 0) {
            data = AllocAPointer(size);
            if (data != 0 && fread(data, 1, size, fp) != size) {
                DeallocAPointer(data);
                data = 0;
                size = 0;
                NonFatal("Error in loading file\n");
            }
        } else {
            NonFatal("Empty file\n");
        }
        fclose(fp);
    }
    if (length != 0)
        *length = size;
    return data;
}

/* The length of an open file, its position left where it was. */
// FUNCTION: EVNOVA 0x004d9920
ULong fgetfilesize(FILE *fp)
{
    int32_t here;
    int32_t length;

    here = ftell(fp);
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, here, SEEK_SET);
    return length;
}
