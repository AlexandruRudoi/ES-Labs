#include "Stats.h"

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// Internal state
static uint32_t s_total = 0;
static uint32_t s_short = 0;
static uint32_t s_long = 0;
static uint32_t s_shortMs = 0;
static uint32_t s_longMs = 0;
static SemaphoreHandle_t s_mutex = NULL;

void statsInit(void)
{
    s_mutex = xSemaphoreCreateMutex();
}

// Acquires mutex, updates counters, releases mutex.
void statsSetPress(bool isShort, uint32_t durationMs)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    s_total++;
    if (isShort)
    {
        s_short++;
        s_shortMs += durationMs;
    }
    else
    {
        s_long++;
        s_longMs += durationMs;
    }

    xSemaphoreGive(s_mutex);
}

// Acquires mutex, reads + resets all counters atomically.
void statsGetAndReset(uint32_t *total,
                      uint32_t *shortCnt,
                      uint32_t *longCnt,
                      uint32_t *avgMs)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    *total = s_total;
    *shortCnt = s_short;
    *longCnt = s_long;

    uint32_t totalMs = s_shortMs + s_longMs;
    *avgMs = (s_total > 0) ? (totalMs / s_total) : 0;

    s_total = 0;
    s_short = 0;
    s_long = 0;
    s_shortMs = 0;
    s_longMs = 0;

    xSemaphoreGive(s_mutex);
}