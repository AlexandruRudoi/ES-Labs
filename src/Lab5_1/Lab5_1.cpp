#include "Lab5_1.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>

#include <Lcd.h>
#include <Led.h>
#include <Buzzer.h>
#include <MotorDriver.h>
#include <DhtSensor.h>

// Pin assignments
#define GREEN_LED_PIN   4   // cooling ON indicator
#define RED_LED_PIN     5   // cooling OFF indicator

// Task periods
#define T1_PERIOD_MS     50UL   // sensor acquisition
#define T2_PERIOD_MS    100UL   // controller
#define T3_PERIOD_MS    500UL   // display + plotter

// Default ON-OFF hysteresis parameters
#define DEFAULT_SETPOINT    20.0f   // desired temperature (C)
#define DEFAULT_HYSTERESIS   1.5f   // +/- band around setpoint

// Motor speed zones
#define SPEED_LOW           50      // % when temp is just above V_on
#define SPEED_HIGH         100      // % when temp > V_on + ZONE_OFFSET
#define ZONE_OFFSET          3.0f   // degrees above V_on for high speed

#define CMD_BUF_SIZE        32

// Shared report: T1 writes sensor, T2 writes controller, T3 reads
typedef struct
{
    float   temperature;    // current temperature (C)
    float   humidity;       // current humidity (%RH)
    bool    sensorValid;    // at least one good DHT read
} SensorReport;

typedef struct
{
    float   setpoint;       // target temperature
    float   hystHigh;       // V_off: turn motor OFF below this
    float   hystLow;        // V_on:  turn motor ON above this
    bool    motorOn;        // current motor state
    uint8_t speedPercent;   // current motor speed (0, 50, or 100)
} ControlReport;

static SemaphoreHandle_t s_reportMutex = NULL;
static SensorReport      s_sensorRpt;
static ControlReport     s_controlRpt;

// Controller state (only accessed by T2)
static float   s_setpoint    = DEFAULT_SETPOINT;
static float   s_hysteresis  = DEFAULT_HYSTERESIS;
static bool    s_motorState  = false;

// Setpoint adjustment queue (serial commands -> T2)
static QueueHandle_t s_cmdQueue = NULL;

typedef enum
{
    CMD_NONE = 0,
    CMD_SET_TEMP,       // set setpoint
    CMD_SET_HYST,       // set hysteresis band
    CMD_STATUS
} CmdType;

typedef struct
{
    CmdType type;
    float   value;
} CmdMsg;

static Led *greenLed = NULL;
static Led *redLed   = NULL;

// Non-blocking serial line accumulator
static char    s_lineBuf[CMD_BUF_SIZE];
static uint8_t s_lineIdx = 0;

static bool serialLineReady(void)
{
    while (Serial.available() > 0)
    {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (s_lineIdx == 0) continue;
            s_lineBuf[s_lineIdx] = '\0';
            return true;
        }
        if (s_lineIdx < CMD_BUF_SIZE - 1)
            s_lineBuf[s_lineIdx++] = c;
    }
    return false;
}

// T1 - Sensor Acquisition + Serial Command Parser (50 ms, prio 2)
static void taskAcquisition(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t period    = pdMS_TO_TICKS(T1_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    char cmd[CMD_BUF_SIZE];
    int  ival;

    for (;;)
    {
        // Read DHT sensor (internally rate-limited to 1s)
        dhtRead();

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_sensorRpt.temperature = dhtGetTemperature();
        s_sensorRpt.humidity    = dhtGetHumidity();
        s_sensorRpt.sensorValid = dhtIsValid();
        xSemaphoreGive(s_reportMutex);

        // Parse serial commands
        if (serialLineReady())
        {
            s_lineIdx = 0;

            int n = 0;
            if (sscanf(s_lineBuf, "%15s%n", cmd, &n) == 1)
            {
                for (uint8_t i = 0; cmd[i]; i++)
                    cmd[i] = (char)tolower((unsigned char)cmd[i]);

                if (strcmp(cmd, "set") == 0)
                {
                    // "set 30" -> setpoint = 30 C
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_TEMP, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else
                        Serial.println(F("Usage: set <temp>"));
                }
                else if (strcmp(cmd, "hyst") == 0)
                {
                    // "hyst 2" -> hysteresis = +/- 2 C
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_HYST, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else
                        Serial.println(F("Usage: hyst <degrees>"));
                }
                else if (strcmp(cmd, "status") == 0)
                {
                    CmdMsg msg = { CMD_STATUS, 0.0f };
                    xQueueSend(s_cmdQueue, &msg, 0);
                }
                else
                {
                    Serial.print(F("Unknown: "));
                    Serial.println(cmd);
                }
            }
        }

        vTaskDelayUntil(&xLastWake, period);
    }
}

// T2 - ON-OFF Controller with Hysteresis (100 ms, prio 3)
static void taskController(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t period    = pdMS_TO_TICKS(T2_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    for (;;)
    {
        // Process any pending commands
        CmdMsg msg;
        while (xQueueReceive(s_cmdQueue, &msg, 0) == pdTRUE)
        {
            switch (msg.type)
            {
            case CMD_SET_TEMP:
                s_setpoint = msg.value;
                buzzerBeep(80);
                Serial.print(F("Setpoint -> "));
                Serial.println((int)s_setpoint);
                break;

            case CMD_SET_HYST:
                if (msg.value > 0.0f)
                {
                    s_hysteresis = msg.value;
                    buzzerBeep(80);
                    Serial.print(F("Hysteresis -> +/- "));
                    Serial.println((int)s_hysteresis);
                }
                break;

            case CMD_STATUS:
            {
                float vOn  = s_setpoint + s_hysteresis;
                float vOff = s_setpoint - s_hysteresis;
                Serial.print(F("SP="));
                Serial.print((int)s_setpoint);
                Serial.print(F(" Hyst="));
                Serial.print((int)s_hysteresis);
                Serial.print(F(" V_on="));
                Serial.print((int)vOn);
                Serial.print(F(" V_off="));
                Serial.println((int)vOff);
                break;
            }

            default:
                break;
            }
        }

        // Read current temperature from shared report
        float temp;
        bool  valid;
        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        temp  = s_sensorRpt.temperature;
        valid = s_sensorRpt.sensorValid;
        xSemaphoreGive(s_reportMutex);

        if (!valid)
        {
            vTaskDelayUntil(&xLastWake, period);
            continue;
        }

        // ON-OFF control with hysteresis
        float vOn  = s_setpoint + s_hysteresis;   // motor ON above this
        float vOff = s_setpoint - s_hysteresis;   // motor OFF below this

        if (temp >= vOn)
            s_motorState = true;
        else if (temp <= vOff)
            s_motorState = false;
        // else: keep current state (hysteresis band)

        // Apply motor output with speed zones
        uint8_t speed = 0;
        if (s_motorState)
        {
            if (temp >= vOn + ZONE_OFFSET)
                speed = SPEED_HIGH;
            else
                speed = SPEED_LOW;

            motorSetPercent((float)speed);
        }
        else
        {
            speed = 0;
            motorStop();
        }

        // LED indicators
        if (s_motorState)
        {
            greenLed->turnOn();
            redLed->turnOff();
        }
        else
        {
            greenLed->turnOff();
            redLed->turnOn();
        }

        // Update control report
        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_controlRpt.setpoint     = s_setpoint;
        s_controlRpt.hystHigh     = vOn;
        s_controlRpt.hystLow      = vOff;
        s_controlRpt.motorOn      = s_motorState;
        s_controlRpt.speedPercent = speed;
        xSemaphoreGive(s_reportMutex);

        vTaskDelayUntil(&xLastWake, period);
    }
}

// T3 - Display + Serial Plotter (500 ms, prio 1)
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1000));
    uint8_t page = 0;

    for (;;)
    {
        SensorReport  sr;
        ControlReport cr;

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        sr = s_sensorRpt;
        cr = s_controlRpt;
        xSemaphoreGive(s_reportMutex);

        if (page == 0)
        {
            // Page 0: Temperature + motor state
            int tInt  = (int)sr.temperature;
            int tFrac = ((int)(sr.temperature * 10.0f)) % 10;
            if (tFrac < 0) tFrac = -tFrac;

            lcdSetCursor(0, 0);
            printf("T:%d.%d SP:%dC  ",
                   tInt, tFrac, (int)cr.setpoint);
            lcdSetCursor(0, 1);
            printf("Fan:%-3s Spd:%3d%%",
                   cr.motorOn ? "ON" : "OFF",
                   cr.speedPercent);
        }
        else
        {
            // Page 1: Hysteresis band + humidity
            int hInt  = (int)sr.humidity;
            int hFrac = ((int)(sr.humidity * 10.0f)) % 10;
            if (hFrac < 0) hFrac = -hFrac;

            lcdSetCursor(0, 0);
            printf("On>%dC Off<%dC  ",
                   (int)cr.hystHigh, (int)cr.hystLow);
            lcdSetCursor(0, 1);
            printf("Hum:%d.%d%%      ",
                   hInt, hFrac);
        }
        page ^= 1;

        // Serial Plotter (Teleplot format: >name:value,name:value,...)
        int tPlot = (int)sr.temperature;
        int tPlotF = ((int)(sr.temperature * 10.0f)) % 10;
        if (tPlotF < 0) tPlotF = -tPlotF;

        Serial.print(F(">Temp:"));
        Serial.print(tPlot);
        Serial.print('.');
        Serial.print(tPlotF);
        Serial.print(F(",SetPoint:"));
        Serial.print((int)cr.setpoint);
        Serial.print(F(",V_on:"));
        Serial.print((int)cr.hystHigh);
        Serial.print(F(",V_off:"));
        Serial.print((int)cr.hystLow);
        Serial.print(F(",Fan:"));
        Serial.print(cr.motorOn ? 1 : 0);
        Serial.print(F(",Speed:"));
        Serial.println(cr.speedPercent);

        vTaskDelay(pdMS_TO_TICKS(T3_PERIOD_MS));
    }
}

// Setup
void lab51Setup(void)
{
    Serial.begin(9600);
    lcdInit();

    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    buzzerInit();
    motorInit();
    dhtInit();

    redLed->turnOn();   // motor initially OFF

    // Init shared reports
    memset(&s_sensorRpt, 0, sizeof(s_sensorRpt));
    memset(&s_controlRpt, 0, sizeof(s_controlRpt));
    s_controlRpt.setpoint = DEFAULT_SETPOINT;
    s_controlRpt.hystHigh = DEFAULT_SETPOINT + DEFAULT_HYSTERESIS;
    s_controlRpt.hystLow  = DEFAULT_SETPOINT - DEFAULT_HYSTERESIS;

    // FreeRTOS objects
    s_reportMutex = xSemaphoreCreateMutex();
    s_cmdQueue    = xQueueCreate(4, sizeof(CmdMsg));

    // Create tasks
    xTaskCreate(taskAcquisition, "T1-Acq",  512, NULL, 2, NULL);
    xTaskCreate(taskController,  "T2-Ctrl", 256, NULL, 3, NULL);
    xTaskCreate(taskDisplay,     "T3-Disp", 768, NULL, 1, NULL);

    lcdClear();
    printf("Lab 5.1 ready");
    Serial.println(F("Lab 5.1 - ON-OFF Temp Control with Hysteresis"));
    Serial.println(F("Commands: set <temp>  hyst <degrees>  status"));

    vTaskStartScheduler();
}

void lab51Loop(void)
{
    // Not reached - scheduler runs in lab51Setup()
}
