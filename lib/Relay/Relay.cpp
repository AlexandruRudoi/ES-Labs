#include "Relay.h"

static bool s_relayState = false;

static void relayWritePin(bool logicalOn)
{
#if RELAY_ACTIVE_LOW
    digitalWrite(RELAY_PIN, logicalOn ? LOW : HIGH);
#else
    digitalWrite(RELAY_PIN, logicalOn ? HIGH : LOW);
#endif
}

void relayInit(void)
{
    pinMode(RELAY_PIN, OUTPUT);
    s_relayState = false;
    relayWritePin(false);
}

void relayOn(void)
{
    s_relayState = true;
    relayWritePin(true);
}

void relayOff(void)
{
    s_relayState = false;
    relayWritePin(false);
}

void relayToggle(void)
{
    if (s_relayState)
        relayOff();
    else
        relayOn();
}

bool relayGetState(void)
{
    return s_relayState;
}
