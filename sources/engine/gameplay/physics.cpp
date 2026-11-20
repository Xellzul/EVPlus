/* physics.cpp -- ship motion helpers (win 0x0043b4a0..0x0043ba30: Accel, AdjustedAccel, CalcInterceptAngle, SpeedLimit ...). Notes: Research/notes/physics.md */
#include <math.h>

#include "game.h"
#include "space.h"

/* TableSin/TableCos look up the 360-entry tables BuildTrigTable fills and leave the result in trigResult. */

/* CalcInterceptAngle returns an int; its callers narrow it to short for Accel. */

// FUNCTION: EVNOVA 0x0043b4a0
// MAC: _Accel i386 0x0002ade6 ppc 0x0002f31c
void Accel(short angle, float mag, FloatPoint *out)
{
    TableSin(angle);
    out->x += trigResult * mag;
    TableCos(angle);
    out->y -= trigResult * mag;
}

// FUNCTION: EVNOVA 0x0043b4e0
// MAC: _AdjustedAccel i386 0x0002ae4b ppc 0x0002f390
void AdjustedAccel(short angle, float accel, FloatPoint *vel, float maxSpeed)
{
    FloatPoint max;
    FloatPoint delta;

    max.x = 0.0f;
    max.y = 0.0f;
    delta.x = 0.0f;
    delta.y = 0.0f;
    Accel(angle, maxSpeed, &max);
    Accel(angle, accel, &delta);
    if (delta.x > 0.0 && max.x > 0.0) {
        if (vel->x < max.x)
            vel->x += delta.x;
    } else if (delta.x < 0.0 && max.x < 0.0) {
        if (vel->x > max.x)
            vel->x += delta.x;
    } else {
        vel->x += delta.x;
    }
    if (delta.y > 0.0 && max.y > 0.0) {
        if (vel->y < max.y)
            vel->y += delta.y;
    } else if (delta.y < 0.0 && max.y < 0.0) {
        if (vel->y > max.y)
            vel->y += delta.y;
    } else {
        vel->y += delta.y;
    }
}

#define FloatAbs(x) fabs(x)

/* the heading from b to a, in degrees */
// FUNCTION: EVNOVA 0x0043b670
// MAC: _CalcInterceptAngle i386 0x0002af60 ppc 0x0002f4e8
short CalcInterceptAngle(FloatPoint *a, FloatPoint *b)
{
    return TableArcTan(a->x - b->x, a->y - b->y);
}

/* clamp both components into [-mag, mag] */
// FUNCTION: EVNOVA 0x0043b6a0
// MAC: _SpeedLimit i386 0x0002af91 ppc 0x0002f528
void SpeedLimit(FloatPoint *p, float mag)
{
    if (p->x > mag)
        p->x = mag;
    if (p->x < -mag)
        p->x = -mag;
    if (p->y > mag)
        p->y = mag;
    if (p->y < -mag)
        p->y = -mag;
}

// FUNCTION: EVNOVA 0x0043b710
// MAC: _XYFloatSquaredDist i386 0x0002afdb ppc 0x0002f570
float XYFloatSquaredDist(FloatPoint *a, FloatPoint *b)
{
    float dx;
    float dy;

    dx = FloatAbs(a->x - b->x);
    dy = FloatAbs(a->y - b->y);
    return dx * dx + dy * dy;
}


// FUNCTION: EVNOVA 0x0043b740
// MAC: _CalcLeadAngle i386 0x0002b025 ppc 0x0002f5b4
short CalcLeadAngle(ShipObj *self, ShipObj *target, short weaponType,
                   FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, &target->pos);
    if (weaponType != -1) {
        short guidance = weaponStats[weaponType].guidance;
        if (weaponStats[weaponType].guidance == kGuidanceUnguided ||
            weaponStats[weaponType].guidance == kGuidanceTurret ||
            weaponStats[weaponType].guidance == kGuidanceRocket ||
            weaponStats[weaponType].guidance == kGuidanceFrontQuad ||
            weaponStats[weaponType].guidance == kGuidanceRearQuad ||
            weaponStats[weaponType].guidance == kGuidancePointDef) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = target->pos.x;
            dx = targetX;
            dy = target->pos.y;
            if (overridePos == 0) {
                dx -= self->pos.x;
                dy -= self->pos.y;
            } else {
                dx -= overridePos->x;
                dy -= overridePos->y;
            }
            dist = sqrtf(dx * dx + dy * dy);
            speed = weaponStats[weaponType].speed100;
            if (guidance == kGuidanceRocket) {
                if (dist <= speed * 19.59)
                    speed = dist / (speed * 0.316);
                else
                    speed = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                speed = dist / speed;
            }
            lead.x = (target->vel.x - self->vel.x) * speed + targetX;
            lead.y = (target->vel.y - self->vel.y) * speed + target->pos.y;
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

// FUNCTION: EVNOVA 0x0043b8c0
// MAC: _CalcObjectLeadAngle i386 0x0002b18e ppc 0x0002f724
short CalcObjectLeadAngle(ShipObj *self, FloatPoint *targetPos, FloatPoint *targetVel,
                   short weaponType, FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, targetPos);
    if (weaponType != -1) {
        short guidance = weaponStats[weaponType].guidance;
        // BUG (all versions): kGuidanceRocket is not in this filter, so the rocket lead-time branch below never
        // runs -- rockets get no lead against asteroids/objects
        if (weaponStats[weaponType].guidance == kGuidanceUnguided ||
            weaponStats[weaponType].guidance == kGuidanceTurret ||
            weaponStats[weaponType].guidance == kGuidanceFrontQuad ||
            weaponStats[weaponType].guidance == kGuidanceRearQuad ||
            weaponStats[weaponType].guidance == kGuidancePointDef) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = targetPos->x;
            dx = targetX;
            dy = targetPos->y;
            if (overridePos != 0) {
                dx -= overridePos->x;
                dy -= overridePos->y;
            } else {
                dx -= self->pos.x;
                dy -= self->pos.y;
            }
            speed = weaponStats[weaponType].speed100;
            dist = sqrtf(dx * dx + dy * dy);
            if (guidance == kGuidanceRocket) {
                if (dist <= speed * 19.59)
                    speed = dist / (speed * 0.316);
                else
                    speed = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                speed = dist / speed;
            }
            lead.x = (targetVel->x - self->vel.x) * speed + targetX;
            lead.y = (targetVel->y - self->vel.y) * speed + targetPos->y;
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

// FUNCTION: EVNOVA 0x0043ba30
// MAC: _CalcSpobLeadAngle i386 0x0002b2f0 ppc 0x0002f878
short CalcSpobLeadAngle(SpobRecord *spob, ShipObj *target)
{
    FloatPoint spobPos;
    short weaponType;
    short angle;

    spobPos.x = spob->pos.h;
    spobPos.y = spob->pos.v;
    angle = CalcInterceptAngle(&spobPos, &target->pos);
    weaponType = spob->weapon;
    if (weaponType != -1) {
        float targetX;
        float dx;
        float dy;
        float speed;
        float dist;
        FloatPoint lead;

        targetX = target->pos.x;
        dx = targetX - spob->pos.h;
        dy = target->pos.y - spob->pos.v;
        speed = weaponStats[weaponType].speed100;
        dist = sqrtf(dx * dx + dy * dy);
        if (weaponStats[weaponType].guidance == kGuidanceRocket) {
            if (dist <= speed * 19.59)
                speed = dist / (speed * 0.316);
            else
                speed = (dist - speed * 19.59) / speed + 2.066666666666667;
        } else {
            speed = dist / speed;
        }
        lead.x = target->vel.x * speed + targetX;
        lead.y = target->vel.y * speed + target->pos.y;
        angle = CalcInterceptAngle(&spobPos, &lead);
    }
    return angle;
}
