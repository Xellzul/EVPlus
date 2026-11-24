/* drawtext.cpp -- centred Pascal-string drawing (win 0x004622f0). Notes: Research/notes/drawtext.md */
/*
 * Both halves of the centring divide by two as signed ints, which is why the
 * exe carries the cmp/sbb/sar rounding idiom twice.
 */
#include "macport.h"
#include "text.h"

// FUNCTION: EVNOVA 0x004622f0
void DrawStringCentered(const unsigned char *str, short left, short right,
                        short v)
{
    short width;

    width = StringWidth(str);
    MoveTo(((left + right) / 2 - width / 2), v);
    DrawString(str);
}

void DrawStringCentered(const std::string &text, short left, short right, short v)
{
    short width;

    width = MeasureCString(UpToNul(text));
    MoveTo(((left + right) / 2 - width / 2), v);
    DrawCString(text);
}
