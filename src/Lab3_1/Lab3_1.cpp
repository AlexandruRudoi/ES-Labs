#include "Lab3_1.h"

#include <Arduino.h>
#include <stdio.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include <Lcd.h>
#include <Led.h>
#include <DhtSensor.h>
#include <NtcSensor.h>
#include <BinaryConditioner.h>
#include <Buzzer.h>

// --- Pin assignments ---
#define GREEN_LED_PIN   4   // ON = all OK
#define RED_LED_PIN     5   // ON = alert active

// --- Task timing ---
#define TASK1_PERIOD_MS   50UL    // acquisition poll rate
#define TASK3_PERIOD_MS 1000UL    // display refresh rate

// --- Binary conditioning thresholds ---
// DHT11 temperature channel
#define DHT_TEMP_SAT_MIN   -10.0f
#define DHT_TEMP_SAT_MAX    60.0f
#define DHT_TEMP_THRESH_HI  28.0f   // alert above this
#define DHT_TEMP_THRESH_LO  25.0f   // alert clears below this
#define DHT_TEMP_DEBOUNCE    5      // 5 x 50ms = 250ms

// DHT11 humidity channel
#define DHT_HUM_SAT_MIN     0.0f
#define DHT_HUM_SAT_MAX   100.0f
#define DHT_HUM_THRESH_HI  70.0f
#define DHT_HUM_THRESH_LO  60.0f
#define DHT_HUM_DEBOUNCE    5

// NTC temperature channel
#define NTC_TEMP_SAT_MIN  -10.0f
#define NTC_TEMP_SAT_MAX   60.0f
#define NTC_TEMP_THRESH_HI 28.0f
#define NTC_TEMP_THRESH_LO 25.0f
#define NTC_TEMP_DEBOUNCE   5

// Shared report: written by T2, read by T3, protected by mutex
typedef struct
{
    // DHT11 raw + conditioned values
    float    dhtTempRaw;
    float    dhtTempSat;
    bool     dhtTempAlert;
    float    dhtHumRaw;
    float    dhtHumSat;
    bool     dhtHumAlert;
    bool     dhtValid;

    // NTC conditioned values
    float    ntcTempSat;
    bool     ntcTempAlert;
} SensorReport;

static SensorReport       s_report;
static SemaphoreHandle_t  s_reportMutex = NULL;
static SemaphoreHandle_t  s_newSample   = NULL;  // T1->T2 binary semaphore

// LED handles — constructed in lab31Setup() so pinMode runs after init()
static Led *greenLed = NULL;
static Led *redLed   = NULL;

// Binary conditioners (one per channel)
static BinaryConditioner s_bcDhtTemp;
static BinaryConditioner s_bcDhtHum;
static BinaryConditioner s_bcNtcTemp;

// T1: read sensors every 50ms, signal T2
static void taskAcquisition(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period       = pdMS_TO_TICKS(TASK1_PERIOD_MS);
    TickType_t       xLastWake    = xTaskGetTickCount();

    for (;;)
    {
        dhtRead();
        ntcRead();

        xSemaphoreGive(s_newSample);
        vTaskDelayUntil(&xLastWake, period);
    }
}

// T2: condition signals, update outputs, pass data to T3
static void taskConditioning(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        if (xSemaphoreTake(s_newSample, portMAX_DELAY) != pdTRUE)
            continue;

        // DHT temp
        float dhtT   = dhtGetTemperature();
        bool  bcDhtT = bcUpdate(&s_bcDhtTemp, dhtT);

        // DHT humidity
        float dhtH   = dhtGetHumidity();
        bool  bcDhtH = bcUpdate(&s_bcDhtHum, dhtH);

        // NTC temp
        float ntcT   = ntcGetTemperature();
        bool  bcNtcT = bcUpdate(&s_bcNtcTemp, ntcT);

        // beep on new alert edge
        if (bcIsNewAlert(&s_bcDhtTemp) ||
            bcIsNewAlert(&s_bcDhtHum)  ||
            bcIsNewAlert(&s_bcNtcTemp))
        {
            buzzerBeep(150);
        }

        // update LEDs
        bool anyAlert = bcDhtT || bcDhtH || bcNtcT;
        if (anyAlert)
        {
            redLed->turnOn();
            greenLed->turnOff();
        }
        else
        {
            greenLed->turnOn();
            redLed->turnOff();
        }

        // write report
        xSemaphoreTake(s_reportMutex, portMAX_DELAY);

        s_report.dhtTempRaw   = dhtT;
        s_report.dhtTempSat   = bcGetSaturated(&s_bcDhtTemp);
        s_report.dhtTempAlert = bcDhtT;
        s_report.dhtHumRaw    = dhtH;
        s_report.dhtHumSat    = bcGetSaturated(&s_bcDhtHum);
        s_report.dhtHumAlert  = bcDhtH;
        s_report.dhtValid     = dhtIsValid();

        s_report.ntcTempSat   = bcGetSaturated(&s_bcNtcTemp);
        s_report.ntcTempAlert = bcNtcT;

        xSemaphoreGive(s_reportMutex);
    }
}

// T3: refresh LCD every 1s (row0=DHT11, row1=NTC)
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    // 2s head-start — let T1 collect valid sensor data
    vTaskDelay(pdMS_TO_TICKS(2000));

    for (;;)
    {
        SensorReport r;

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        r = s_report;
        xSemaphoreGive(s_reportMutex);

        // Row 0: DHT11
        lcdSetCursor(0, 0);
        if (r.dhtValid)
        {
            char t[7];
            dtostrf(r.dhtTempSat, 4, 1, t);
            const char *a = (r.dhtTempAlert || r.dhtHumAlert)
                ? (r.dhtTempAlert && r.dhtHumAlert ? "!TH" : (r.dhtTempAlert ? "!T" : "!H"))
                : "OK ";
            printf("D:%-5sC %2d%% %-3s", t, (int)r.dhtHumSat, a);
        }
        else
        {
            printf("D: waiting...   ");
        }

        // Row 1: NTC
        lcdSetCursor(0, 1);
        {
            char t[7];
            dtostrf(r.ntcTempSat, 4, 1, t);
            const char *a = r.ntcTempAlert ? "!T " : "OK ";
            printf("N:%-5sC      %-3s", t, a);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK3_PERIOD_MS));
    }
}

void lab31Setup(void)
{
    Serial.begin(9600);

    lcdInit();   // stdout -> LCD

    // LEDs — must be created here, not as static globals (pinMode before init)
    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    dhtInit();
    ntcInit();
    buzzerInit();

    // Binary conditioner - DHT temperature
    s_bcDhtTemp.minVal      = DHT_TEMP_SAT_MIN;
    s_bcDhtTemp.maxVal      = DHT_TEMP_SAT_MAX;
    s_bcDhtTemp.threshHigh  = DHT_TEMP_THRESH_HI;
    s_bcDhtTemp.threshLow   = DHT_TEMP_THRESH_LO;
    s_bcDhtTemp.debounceMax = DHT_TEMP_DEBOUNCE;
    bcInit(&s_bcDhtTemp);

    // Binary conditioner - DHT humidity
    s_bcDhtHum.minVal      = DHT_HUM_SAT_MIN;
    s_bcDhtHum.maxVal      = DHT_HUM_SAT_MAX;
    s_bcDhtHum.threshHigh  = DHT_HUM_THRESH_HI;
    s_bcDhtHum.threshLow   = DHT_HUM_THRESH_LO;
    s_bcDhtHum.debounceMax = DHT_HUM_DEBOUNCE;
    bcInit(&s_bcDhtHum);

    // Binary conditioner - NTC temperature
    s_bcNtcTemp.minVal      = NTC_TEMP_SAT_MIN;
    s_bcNtcTemp.maxVal      = NTC_TEMP_SAT_MAX;
    s_bcNtcTemp.threshHigh  = NTC_TEMP_THRESH_HI;
    s_bcNtcTemp.threshLow   = NTC_TEMP_THRESH_LO;
    s_bcNtcTemp.debounceMax = NTC_TEMP_DEBOUNCE;
    bcInit(&s_bcNtcTemp);

    // FreeRTOS sync primitives
    s_reportMutex = xSemaphoreCreateMutex();
    s_newSample   = xSemaphoreCreateBinary();

    BaseType_t r1 = xTaskCreate(taskAcquisition,  "T1-Acq",   512, NULL, 2, NULL);
    BaseType_t r2 = xTaskCreate(taskConditioning, "T2-Cond",  512, NULL, 2, NULL);
    BaseType_t r3 = xTaskCreate(taskDisplay,      "T3-Disp",  768, NULL, 1, NULL);
    (void)r1; (void)r2; (void)r3;

    lcdClear();
    printf("Lab 3.1 ready");

    vTaskStartScheduler();
}

void lab31Loop(void)
{
    // Unreachable: vTaskStartScheduler() called in lab31Setup() never returns
}
