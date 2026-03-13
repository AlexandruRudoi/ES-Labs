#include "DhtSensor.h"

#include <DHT.h>

static DHT     s_dht(DHT_PIN, DHT11);
static float   s_temperature = 0.0f;
static float   s_humidity    = 0.0f;
static bool    s_valid       = false;
static uint32_t s_lastReadMs = 0;

void dhtInit(void)
{
    s_dht.begin();
}

void dhtRead(void)
{
    uint32_t now = millis();

    // Enforce minimum 1 s interval between reads (DHT11 spec)
    if (now - s_lastReadMs < DHT_MIN_INTERVAL_MS)
        return;

    s_lastReadMs = now;

    float t = s_dht.readTemperature();
    float h = s_dht.readHumidity();

    // isnan() check — DHT library returns NaN on read failure
    if (!isnan(t) && !isnan(h))
    {
        s_temperature = t;
        s_humidity    = h;
        s_valid       = true;
    }
}

float dhtGetTemperature(void)
{
    return s_temperature;
}

float dhtGetHumidity(void)
{
    return s_humidity;
}

bool dhtIsValid(void)
{
    return s_valid;
}
