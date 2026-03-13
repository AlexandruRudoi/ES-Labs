#include "BinaryConditioner.h"

void bcInit(BinaryConditioner *bc)
{
    bc->saturated  = 0.0f;
    bc->counter    = 0;
    bc->alertState = false;
    bc->newAlert   = false;
}

bool bcUpdate(BinaryConditioner *bc, float value)
{
    // Clamp to [minVal, maxVal]
    if (value < bc->minVal) value = bc->minVal;
    if (value > bc->maxVal) value = bc->maxVal;
    bc->saturated = value;

    // Hysteresis: above threshHigh=ALERT, below threshLow=OK, between=hold
    bool wantAlert;
    if (value >= bc->threshHigh)
        wantAlert = true;
    else if (value <= bc->threshLow)
        wantAlert = false;
    else
        wantAlert = bc->alertState; // inside hysteresis band, no change

    // Debounce: count up/down, latch only at limits
    if (wantAlert)
    {
        if (bc->counter < bc->debounceMax)
            bc->counter++;
    }
    else
    {
        if (bc->counter > 0)
            bc->counter--;
    }

    // Latch and detect rising edge
    bool prevAlert = bc->alertState;

    if (bc->counter >= bc->debounceMax)
        bc->alertState = true;
    else if (bc->counter <= 0)
        bc->alertState = false;

    // Rising edge detection: OK->ALERT
    if (!prevAlert && bc->alertState)
        bc->newAlert = true;

    return bc->alertState;
}

bool bcIsAlert(const BinaryConditioner *bc)
{
    return bc->alertState;
}

bool bcIsNewAlert(BinaryConditioner *bc)
{
    if (bc->newAlert)
    {
        bc->newAlert = false;
        return true;
    }
    return false;
}

float bcGetSaturated(const BinaryConditioner *bc)
{
    return bc->saturated;
}
