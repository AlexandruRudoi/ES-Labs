#include "NtcSensor.h"

static uint16_t s_raw         = 0;
static float    s_temperature = 0.0f;

void ntcInit(void)
{
    pinMode(NTC_PIN, INPUT);
}

void ntcRead(void)
{
    s_raw = (uint16_t)analogRead(NTC_PIN);

    // Two-point linear conversion:
    // T = T1 + (T2 - T1) * (raw - raw1) / (raw2 - raw1)
    s_temperature = NTC_TEMP1 +
                    (NTC_TEMP2 - NTC_TEMP1) *
                    ((float)(s_raw - NTC_RAW1)) /
                    ((float)(NTC_RAW2 - NTC_RAW1));
}

uint16_t ntcGetRaw(void)
{
    return s_raw;
}

float ntcGetTemperature(void)
{
    return s_temperature;
}
