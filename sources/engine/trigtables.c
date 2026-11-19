// COMPILE: -lang c++ -bool off
/* trigtables.c -- angle-indexed trig tables, TableSin/TableCos, and the arctan ramp TableArcTan's atan2-by-table helper walks (win 0x00461830..0x004619b0). Notes: Research/notes/trigtables.md */
#include <math.h>
#include "gameconst.h"
#include "game.h"
#include "space.h"

#define kDegToRad   0.01745329
#define kArctanStep 0.01
#define kRadToDeg   57.2957795

float tableSin[360];
float tableCos[360];
float trigResult;
short tableArcTan[0x400];

// FUNCTION: EVNOVA 0x00461830
// MAC: _BuildTrigTable i386 0x00025828 ppc 0x00029e50
void BuildTrigTable(void)
{
    int32_t i;
    float ang;
    double v;

    for (i = 0; i <= kFullCircle - 1; i++) {
        v = ang = (float)i * kDegToRad;
        tableSin[i] = sin(v);
        tableCos[i] = cos(v);
    }
    for (i = 0; i <= 0x3ff; i++) {
        v = atan(i * kArctanStep);
        v *= kRadToDeg;
        tableArcTan[i] = v;
    }
}


// FUNCTION: EVNOVA 0x00461930
// MAC: _TableSin i386 0x0002590a ppc 0x00029fa0
void TableSin(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = tableSin[angle];
}

// FUNCTION: EVNOVA 0x00461970
// MAC: _TableCos i386 0x0002593c ppc 0x00029fe8
void TableCos(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = tableCos[angle];
}

#define kArctanScale   100.0f
#define kArctanLast    0x3ff

// FUNCTION: EVNOVA 0x004619b0
// MAC: _TableArcTan i386 0x000259a0 ppc 0x0002a078
short TableArcTan(float dx, float dy)
{
    float ratio;
    float ax;
    float ay;
    short idx;
    short ang;

    if (dx == 0.0 && dy == 0.0)
        return 0;

    if (dx < 0.0)
        ax = dx * -1.0;
    else
        ax = dx;
    if (dy < 0.0)
        ay = dy * -1.0;
    else
        ay = dy;
    if (ax < ay)
        ratio = ax / ay;
    else
        ratio = ay / ax;
    if (ratio < 0.0f)
        ratio = -ratio;

    idx = (ratio * kArctanScale);
    if (idx < 0)
        idx = 0;
    if (idx > kArctanLast)
        idx = kArctanLast;

    ang = tableArcTan[idx];
    if (ang < 0)
        ang = -ang;
    if (ax < ay)
        ang = kQuarterCircle - ang;
    if (dx < 0.0f && dy >= 0.0f)
        ang = kHalfCircle - ang;
    if (dx < 0.0f && dy < 0.0f)
        ang = ang + kHalfCircle;
    if (dx >= 0.0f && dy < 0.0f)
        ang = -ang;

    ang -= kQuarterCircle;
    if (ang < 0)
        ang += kFullCircle;
    if (ang >= kFullCircle)
        ang -= kFullCircle;
    return ang;
}
