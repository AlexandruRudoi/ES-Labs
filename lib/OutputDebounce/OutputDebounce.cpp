#include "OutputDebounce.h"

void odInit(OutputDebounce *od)
{
    od->state        = false;
    od->lastChangeMs = millis();
    od->toggleCount  = 0;
    od->rejectCount  = 0;
}

bool odRequest(OutputDebounce *od, bool requestedOn)
{
    // Same state as current - accept as no-op
    if (requestedOn == od->state)
        return true;

    uint32_t now     = millis();
    uint32_t elapsed = now - od->lastChangeMs;

    if (elapsed < od->minHoldMs)
    {
        // Too soon - reject
        od->rejectCount++;
        return false;
    }

    // Accept the change
    od->state        = requestedOn;
    od->lastChangeMs = now;
    od->toggleCount++;
    return true;
}

bool odGetState(const OutputDebounce *od)
{
    return od->state;
}

uint32_t odGetRejections(const OutputDebounce *od)
{
    return od->rejectCount;
}

uint32_t odGetToggleCount(const OutputDebounce *od)
{
    return od->toggleCount;
}

uint32_t odGetHeldMs(const OutputDebounce *od)
{
    return millis() - od->lastChangeMs;
}
