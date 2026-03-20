#include "LdrSensor.h"

static uint16_t s_raw     = 0;
static float    s_percent = 0.0f;

void ldrInit(void)
{
    pinMode(LDR_PIN, INPUT);
}

void ldrRead(void)
{
    s_raw = (uint16_t)analogRead(LDR_PIN);
    s_percent = ((float)s_raw / (float)LDR_ADC_MAX) * 100.0f;
}

uint16_t ldrGetRaw(void)
{
    return s_raw;
}

float ldrGetLightPercent(void)
{
    return s_percent;
}
