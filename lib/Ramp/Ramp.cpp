#include "Ramp.h"

void rampInit(Ramp *r)
{
    r->target = 0.0f;
    r->value  = 0.0f;
}

void rampSetTarget(Ramp *r, float target)
{
    r->target = target;
}

float rampUpdate(Ramp *r, float dt)
{
    float diff = r->target - r->value;

    if (diff > 0.0f)
    {
        float step = r->rateUp * dt;
        r->value += (diff < step) ? diff : step;
    }
    else if (diff < 0.0f)
    {
        float step = r->rateDown * dt;
        float absDiff = -diff;
        r->value -= (absDiff < step) ? absDiff : step;
    }

    return r->value;
}

float rampGetValue(const Ramp *r)
{
    return r->value;
}

float rampGetTarget(const Ramp *r)
{
    return r->target;
}
