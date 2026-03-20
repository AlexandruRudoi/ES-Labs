#include "Lab3_2.h"

#include <Arduino.h>
#include <stdio.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include <Lcd.h>
#include <Led.h>
#include <DhtSensor.h>
#include <NtcSensor.h>
#include <BinaryConditioner.h>
#include <MedianFilter.h>
#include <WeightedAverage.h>
#include <LdrSensor.h>
#include <Buzzer.h>

// Pin assignments
#define GREEN_LED_PIN   4
#define RED_LED_PIN     5

// Task timing
#define TASK1_PERIOD_MS   50UL
#define TASK3_PERIOD_MS 1000UL

// Saturation limits
#define DHT_TEMP_SAT_MIN   -10.0f
#define DHT_TEMP_SAT_MAX    60.0f
#define DHT_HUM_SAT_MIN     0.0f
#define DHT_HUM_SAT_MAX   100.0f
#define NTC_TEMP_SAT_MIN  -10.0f
#define NTC_TEMP_SAT_MAX   60.0f

// LDR light level limits (percentage)
#define LDR_SAT_MIN     0.0f
#define LDR_SAT_MAX   100.0f

// Median filter window sizes
#define MF_WINDOW_DHT_TEMP  5
#define MF_WINDOW_DHT_HUM   5
#define MF_WINDOW_NTC_TEMP   5
#define MF_WINDOW_LDR        5

// EMA smoothing factors
#define EMA_ALPHA_DHT_TEMP  0.25f
#define EMA_ALPHA_DHT_HUM   0.25f
#define EMA_ALPHA_NTC_TEMP   0.25f
#define EMA_ALPHA_LDR        0.25f

// Binary conditioning thresholds
#define DHT_TEMP_THRESH_HI  28.0f
#define DHT_TEMP_THRESH_LO  25.0f
#define DHT_TEMP_DEBOUNCE    5

#define DHT_HUM_THRESH_HI  70.0f
#define DHT_HUM_THRESH_LO  60.0f
#define DHT_HUM_DEBOUNCE    5

#define NTC_TEMP_THRESH_HI 28.0f
#define NTC_TEMP_THRESH_LO 25.0f
#define NTC_TEMP_DEBOUNCE   5

// LDR alert: light drops below threshold = too dark
#define LDR_THRESH_HI  25.0f
#define LDR_THRESH_LO  15.0f
#define LDR_DEBOUNCE    5

// Shared report: written by T2, read by T3
typedef struct
{
    // DHT temperature pipeline stages
    float    dhtTempRaw;
    float    dhtTempSat;
    float    dhtTempMedian;
    float    dhtTempEma;
    bool     dhtTempAlert;

    // DHT humidity pipeline stages
    float    dhtHumRaw;
    float    dhtHumSat;
    float    dhtHumMedian;
    float    dhtHumEma;
    bool     dhtHumAlert;

    // NTC temperature pipeline stages
    float    ntcTempSat;
    float    ntcTempMedian;
    float    ntcTempEma;
    bool     ntcTempAlert;

    // LDR light level pipeline stages
    float    ldrRaw;
    float    ldrSat;
    float    ldrMedian;
    float    ldrEma;
    bool     ldrAlert;     // true = too dark

    bool     dhtValid;
} SensorReport;

static SensorReport       s_report;
static SemaphoreHandle_t  s_reportMutex = NULL;
static SemaphoreHandle_t  s_newSample   = NULL;

static Led *greenLed = NULL;
static Led *redLed   = NULL;

// Pipeline stages per channel
static BinaryConditioner s_bcDhtTemp;
static BinaryConditioner s_bcDhtHum;
static BinaryConditioner s_bcNtcTemp;
static BinaryConditioner s_bcLdr;

static MedianFilter      s_mfDhtTemp;
static MedianFilter      s_mfDhtHum;
static MedianFilter      s_mfNtcTemp;
static MedianFilter      s_mfLdr;

static WeightedAverage   s_waDhtTemp;
static WeightedAverage   s_waDhtHum;
static WeightedAverage   s_waNtcTemp;
static WeightedAverage   s_waLdr;

static uint8_t s_displayPage = 0;  // alternates LCD row 1 between NTC and LDR

// Helper: clamp value to [lo, hi]
static float saturate(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

// T1: read sensors every TASK1_PERIOD_MS, signal T2
static void taskAcquisition(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period    = pdMS_TO_TICKS(TASK1_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    for (;;)
    {
        dhtRead();
        ntcRead();
        ldrRead();

        xSemaphoreGive(s_newSample);
        vTaskDelayUntil(&xLastWake, period);
    }
}

// T2: full conditioning pipeline per channel, update LEDs/buzzer, write report
static void taskConditioning(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        if (xSemaphoreTake(s_newSample, portMAX_DELAY) != pdTRUE)
            continue;

        // DHT temperature
        float dhtTRaw = dhtGetTemperature();
        float dhtTSat = saturate(dhtTRaw, DHT_TEMP_SAT_MIN, DHT_TEMP_SAT_MAX);
        float dhtTMed = mfUpdate(&s_mfDhtTemp, dhtTSat);
        float dhtTEma = waUpdate(&s_waDhtTemp, dhtTMed);
        bool  dhtTAlr = bcUpdate(&s_bcDhtTemp, dhtTEma);

        // DHT humidity
        float dhtHRaw = dhtGetHumidity();
        float dhtHSat = saturate(dhtHRaw, DHT_HUM_SAT_MIN, DHT_HUM_SAT_MAX);
        float dhtHMed = mfUpdate(&s_mfDhtHum, dhtHSat);
        float dhtHEma = waUpdate(&s_waDhtHum, dhtHMed);
        bool  dhtHAlr = bcUpdate(&s_bcDhtHum, dhtHEma);

        // NTC temperature
        float ntcTRaw = ntcGetTemperature();
        float ntcTSat = saturate(ntcTRaw, NTC_TEMP_SAT_MIN, NTC_TEMP_SAT_MAX);
        float ntcTMed = mfUpdate(&s_mfNtcTemp, ntcTSat);
        float ntcTEma = waUpdate(&s_waNtcTemp, ntcTMed);
        bool  ntcTAlr = bcUpdate(&s_bcNtcTemp, ntcTEma);

        // LDR light level (alert = too dark, so inverted logic)
        float ldrRaw  = ldrGetLightPercent();
        float ldrSat  = saturate(ldrRaw, LDR_SAT_MIN, LDR_SAT_MAX);
        float ldrMed  = mfUpdate(&s_mfLdr, ldrSat);
        float ldrEma  = waUpdate(&s_waLdr, ldrMed);
        // Feed inverted value (darkness) so alert triggers when light is low
        bool  ldrAlr  = bcUpdate(&s_bcLdr, 100.0f - ldrEma);

        // Buzzer on new alert edge
        if (bcIsNewAlert(&s_bcDhtTemp) ||
            bcIsNewAlert(&s_bcDhtHum)  ||
            bcIsNewAlert(&s_bcNtcTemp) ||
            bcIsNewAlert(&s_bcLdr))
        {
            buzzerBeep(150);
        }

        // LED output
        bool anyAlert = dhtTAlr || dhtHAlr || ntcTAlr || ldrAlr;
        if (anyAlert) { redLed->turnOn();  greenLed->turnOff(); }
        else          { greenLed->turnOn(); redLed->turnOff();   }

        // Write report with all intermediate values
        xSemaphoreTake(s_reportMutex, portMAX_DELAY);

        s_report.dhtTempRaw    = dhtTRaw;
        s_report.dhtTempSat    = dhtTSat;
        s_report.dhtTempMedian = dhtTMed;
        s_report.dhtTempEma    = dhtTEma;
        s_report.dhtTempAlert  = dhtTAlr;

        s_report.dhtHumRaw     = dhtHRaw;
        s_report.dhtHumSat     = dhtHSat;
        s_report.dhtHumMedian  = dhtHMed;
        s_report.dhtHumEma     = dhtHEma;
        s_report.dhtHumAlert   = dhtHAlr;

        s_report.ntcTempSat    = ntcTSat;
        s_report.ntcTempMedian = ntcTMed;
        s_report.ntcTempEma    = ntcTEma;
        s_report.ntcTempAlert  = ntcTAlr;

        s_report.ldrRaw        = ldrRaw;
        s_report.ldrSat        = ldrSat;
        s_report.ldrMedian     = ldrMed;
        s_report.ldrEma        = ldrEma;
        s_report.ldrAlert      = ldrAlr;

        s_report.dhtValid      = dhtIsValid();

        xSemaphoreGive(s_reportMutex);
    }
}

// T3: display pipeline stages on LCD + Serial
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(2000));

    for (;;)
    {
        SensorReport r;

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        r = s_report;
        xSemaphoreGive(s_reportMutex);

        // Serial: show all pipeline stages
        Serial.println(F("--- Pipeline ---"));

        if (r.dhtValid)
        {
            Serial.print(F("DHT T: raw="));  Serial.print(r.dhtTempRaw, 1);
            Serial.print(F(" sat="));         Serial.print(r.dhtTempSat, 1);
            Serial.print(F(" med="));         Serial.print(r.dhtTempMedian, 1);
            Serial.print(F(" ema="));         Serial.print(r.dhtTempEma, 1);
            Serial.println(r.dhtTempAlert ? F(" ALERT") : F(" ok"));

            Serial.print(F("DHT H: raw="));  Serial.print(r.dhtHumRaw, 1);
            Serial.print(F(" sat="));         Serial.print(r.dhtHumSat, 1);
            Serial.print(F(" med="));         Serial.print(r.dhtHumMedian, 1);
            Serial.print(F(" ema="));         Serial.print(r.dhtHumEma, 1);
            Serial.println(r.dhtHumAlert ? F(" ALERT") : F(" ok"));
        }
        else
        {
            Serial.println(F("DHT: waiting..."));
        }

        Serial.print(F("NTC T: "));
        Serial.print(F("sat="));  Serial.print(r.ntcTempSat, 1);
        Serial.print(F(" med=")); Serial.print(r.ntcTempMedian, 1);
        Serial.print(F(" ema=")); Serial.print(r.ntcTempEma, 1);
        Serial.println(r.ntcTempAlert ? F(" ALERT") : F(" ok"));

        Serial.print(F("LDR L: raw=")); Serial.print(r.ldrRaw, 1);
        Serial.print(F(" sat="));        Serial.print(r.ldrSat, 1);
        Serial.print(F(" med="));        Serial.print(r.ldrMedian, 1);
        Serial.print(F(" ema="));        Serial.print(r.ldrEma, 1);
        Serial.println(r.ldrAlert ? F(" ALERT(dark)") : F(" ok"));

        // Serial Plotter format: label:value pairs, tab-separated
        Serial.print(F("DHT_raw:"));  Serial.print(r.dhtTempRaw, 1);    Serial.print('\t');
        Serial.print(F("DHT_sat:"));  Serial.print(r.dhtTempSat, 1);    Serial.print('\t');
        Serial.print(F("DHT_med:"));  Serial.print(r.dhtTempMedian, 1); Serial.print('\t');
        Serial.print(F("DHT_ema:"));  Serial.print(r.dhtTempEma, 1);    Serial.print('\t');
        Serial.print(F("NTC_sat:"));  Serial.print(r.ntcTempSat, 1);    Serial.print('\t');
        Serial.print(F("NTC_med:"));  Serial.print(r.ntcTempMedian, 1); Serial.print('\t');
        Serial.print(F("NTC_ema:"));  Serial.print(r.ntcTempEma, 1);    Serial.print('\t');
        Serial.print(F("LDR_raw:"));  Serial.print(r.ldrRaw, 1);        Serial.print('\t');
        Serial.print(F("LDR_ema:"));  Serial.println(r.ldrEma, 1);

        // Alert summary
        Serial.print(F("Alerts: DhtT="));
        Serial.print(r.dhtTempAlert ? F("YES") : F("no"));
        Serial.print(F(" DhtH="));
        Serial.print(r.dhtHumAlert ? F("YES") : F("no"));
        Serial.print(F(" NtcT="));
        Serial.print(r.ntcTempAlert ? F("YES") : F("no"));
        Serial.print(F(" Ldr="));
        Serial.println(r.ldrAlert ? F("YES") : F("no"));

        // LCD row 0: DHT final values
        lcdSetCursor(0, 0);
        if (r.dhtValid)
        {
            char t[7];
            dtostrf(r.dhtTempEma, 4, 1, t);
            const char *a = (r.dhtTempAlert || r.dhtHumAlert)
                ? (r.dhtTempAlert && r.dhtHumAlert ? "!TH" : (r.dhtTempAlert ? "!T" : "!H"))
                : "OK ";
            printf("D:%-5sC %2d%% %-3s", t, (int)r.dhtHumEma, a);
        }
        else
        {
            printf("D: waiting...   ");
        }

        // LCD row 1: alternate NTC / LDR each refresh
        lcdSetCursor(0, 1);
        if (s_displayPage == 0)
        {
            char t[7];
            dtostrf(r.ntcTempEma, 4, 1, t);
            const char *a = r.ntcTempAlert ? "!T " : "OK ";
            printf("N:%-5sC      %-3s", t, a);
        }
        else
        {
            const char *a = r.ldrAlert ? "!L " : "OK ";
            printf("L:%3d%%         %-3s", (int)r.ldrEma, a);
        }
        s_displayPage = 1 - s_displayPage;

        vTaskDelay(pdMS_TO_TICKS(TASK3_PERIOD_MS));
    }
}

void lab32Setup(void)
{
    Serial.begin(9600);
    lcdInit();

    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    dhtInit();
    ntcInit();
    ldrInit();
    buzzerInit();

    // Median filters
    s_mfDhtTemp.windowSize = MF_WINDOW_DHT_TEMP;
    mfInit(&s_mfDhtTemp);

    s_mfDhtHum.windowSize = MF_WINDOW_DHT_HUM;
    mfInit(&s_mfDhtHum);

    s_mfNtcTemp.windowSize = MF_WINDOW_NTC_TEMP;
    mfInit(&s_mfNtcTemp);

    s_mfLdr.windowSize = MF_WINDOW_LDR;
    mfInit(&s_mfLdr);

    // EMA filters
    s_waDhtTemp.alpha = EMA_ALPHA_DHT_TEMP;
    waInit(&s_waDhtTemp);

    s_waDhtHum.alpha = EMA_ALPHA_DHT_HUM;
    waInit(&s_waDhtHum);

    s_waNtcTemp.alpha = EMA_ALPHA_NTC_TEMP;
    waInit(&s_waNtcTemp);

    s_waLdr.alpha = EMA_ALPHA_LDR;
    waInit(&s_waLdr);

    // Binary conditioners (alert thresholds)
    s_bcDhtTemp.minVal      = DHT_TEMP_SAT_MIN;
    s_bcDhtTemp.maxVal      = DHT_TEMP_SAT_MAX;
    s_bcDhtTemp.threshHigh  = DHT_TEMP_THRESH_HI;
    s_bcDhtTemp.threshLow   = DHT_TEMP_THRESH_LO;
    s_bcDhtTemp.debounceMax = DHT_TEMP_DEBOUNCE;
    bcInit(&s_bcDhtTemp);

    s_bcDhtHum.minVal      = DHT_HUM_SAT_MIN;
    s_bcDhtHum.maxVal      = DHT_HUM_SAT_MAX;
    s_bcDhtHum.threshHigh  = DHT_HUM_THRESH_HI;
    s_bcDhtHum.threshLow   = DHT_HUM_THRESH_LO;
    s_bcDhtHum.debounceMax = DHT_HUM_DEBOUNCE;
    bcInit(&s_bcDhtHum);

    s_bcNtcTemp.minVal      = NTC_TEMP_SAT_MIN;
    s_bcNtcTemp.maxVal      = NTC_TEMP_SAT_MAX;
    s_bcNtcTemp.threshHigh  = NTC_TEMP_THRESH_HI;
    s_bcNtcTemp.threshLow   = NTC_TEMP_THRESH_LO;
    s_bcNtcTemp.debounceMax = NTC_TEMP_DEBOUNCE;
    bcInit(&s_bcNtcTemp);

    // Binary conditioner - LDR (inverted: 100-light%, alert when dark)
    s_bcLdr.minVal      = LDR_SAT_MIN;
    s_bcLdr.maxVal      = LDR_SAT_MAX;
    s_bcLdr.threshHigh  = 100.0f - LDR_THRESH_LO;  // darkness > 85% = alert
    s_bcLdr.threshLow   = 100.0f - LDR_THRESH_HI;  // darkness < 75% = clear
    s_bcLdr.debounceMax = LDR_DEBOUNCE;
    bcInit(&s_bcLdr);

    // FreeRTOS sync
    s_reportMutex = xSemaphoreCreateMutex();
    s_newSample   = xSemaphoreCreateBinary();

    BaseType_t r1 = xTaskCreate(taskAcquisition,  "T1-Acq",   512, NULL, 2, NULL);
    BaseType_t r2 = xTaskCreate(taskConditioning, "T2-Cond",  512, NULL, 2, NULL);
    BaseType_t r3 = xTaskCreate(taskDisplay,      "T3-Disp",  768, NULL, 1, NULL);
    (void)r1; (void)r2; (void)r3;

    lcdClear();
    printf("Lab 3.2 ready");

    vTaskStartScheduler();
}

void lab32Loop(void)
{
    // Unreachable: vTaskStartScheduler() called in lab32Setup() never returns
}
