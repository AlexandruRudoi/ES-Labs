#include "WeightedAverage.h"

void waInit(WeightedAverage *wa)
{
    wa->value  = 0.0f;
    wa->primed = false;
}

float waUpdate(WeightedAverage *wa, float value)
{
    if (!wa->primed)
    {
        wa->value  = value;
        wa->primed = true;
    }
    else
    {
        wa->value = wa->alpha * value + (1.0f - wa->alpha) * wa->value;
    }

    return wa->value;
}

float waGetValue(const WeightedAverage *wa)
{
    return wa->value;
}
