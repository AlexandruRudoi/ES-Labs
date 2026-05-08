#include "Lab5_2.h"

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
#include <Relay.h>
#include <MotorDriver.h>
#include <DhtSensor.h>

// Pin assignments
#define GREEN_LED_PIN   4   // fan ON indicator
#define RED_LED_PIN     5   // heater ON indicator

// Task periods
#define T1_PERIOD_MS     50UL   // sensor acquisition
#define T2_PERIOD_MS    100UL   // PID controller
#define T3_PERIOD_MS    500UL   // display + plotter

// Default PID parameters
#define DEFAULT_SETPOINT    30.0f   // target temperature (C)
#define DEFAULT_KP          10.0f   // proportional gain
#define DEFAULT_KI           0.1f   // integral gain
#define DEFAULT_KD           5.0f   // derivative gain

// PID output clamping
#define PID_OUT_MIN       -100.0f   // negative = heating needed
#define PID_OUT_MAX        100.0f   // positive = cooling needed

// Integral wind-up limit
#define INTEGRAL_LIMIT     200.0f

#define CMD_BUF_SIZE        32

// dt in seconds for the controller (T2 period)
#define DT_S   (T2_PERIOD_MS / 1000.0f)

namespace {
struct SensorReport
{
    float temperature;
    float humidity;
    bool  sensorValid;
};
struct ControlReport
{
    float   setpoint;
    float   kp;
    float   ki;
    float   kd;
    float   error;
    float   pidOutput;      // -100 to +100
    bool    fanOn;
    uint8_t fanSpeed;       // 0-100 %
    bool    heaterOn;
};
enum CmdType
{
    CMD_NONE = 0,
    CMD_SET_TEMP,
    CMD_SET_KP,
    CMD_SET_KI,
    CMD_SET_KD,
    CMD_STATUS,
    CMD_RESET_PID
};
struct CmdMsg
{
    CmdType type;
    float   value;
};
} // namespace

static SemaphoreHandle_t s_reportMutex = NULL;
static SensorReport      s_sensorRpt;
static ControlReport     s_controlRpt;

// PID state (only accessed by T2)
static float s_setpoint  = DEFAULT_SETPOINT;
static float s_kp        = DEFAULT_KP;
static float s_ki        = DEFAULT_KI;
static float s_kd        = DEFAULT_KD;
static float s_integral  = 0.0f;
static float s_prevError = 0.0f;

// Command queue
static QueueHandle_t s_cmdQueue = NULL;

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
            s_lineIdx = 0;
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
        // Read DHT sensor (driver internally rate-limits to 1 s)
        dhtRead();

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_sensorRpt.temperature = dhtGetTemperature();
        s_sensorRpt.humidity    = dhtGetHumidity();
        s_sensorRpt.sensorValid = dhtIsValid();
        xSemaphoreGive(s_reportMutex);

        // Parse serial commands
        if (serialLineReady())
        {
            int n = 0;
            if (sscanf(s_lineBuf, "%15s%n", cmd, &n) == 1)
            {
                for (uint8_t i = 0; cmd[i]; i++)
                    cmd[i] = (char)tolower((unsigned char)cmd[i]);

                if (strcmp(cmd, "set") == 0)
                {
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_TEMP, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else Serial.println(F("Usage: set <temp>"));
                }
                else if (strcmp(cmd, "kp") == 0)
                {
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_KP, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else Serial.println(F("Usage: kp <value>"));
                }
                else if (strcmp(cmd, "ki") == 0)
                {
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_KI, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else Serial.println(F("Usage: ki <value>"));
                }
                else if (strcmp(cmd, "kd") == 0)
                {
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        CmdMsg msg = { CMD_SET_KD, (float)ival };
                        xQueueSend(s_cmdQueue, &msg, 0);
                    }
                    else Serial.println(F("Usage: kd <value>"));
                }
                else if (strcmp(cmd, "reset") == 0)
                {
                    CmdMsg msg = { CMD_RESET_PID, 0.0f };
                    xQueueSend(s_cmdQueue, &msg, 0);
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

// T2 - PID Controller (100 ms, prio 3)
static void taskController(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t period    = pdMS_TO_TICKS(T2_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    for (;;)
    {
        // Drain command queue
        CmdMsg msg;
        while (xQueueReceive(s_cmdQueue, &msg, 0) == pdTRUE)
        {
            switch (msg.type)
            {
            case CMD_SET_TEMP:
                s_setpoint  = msg.value;
                s_integral  = 0.0f;     // reset integral on setpoint change
                s_prevError = 0.0f;
                buzzerBeep(80);
                Serial.print(F("Setpoint -> "));
                Serial.println((int)s_setpoint);
                break;

            case CMD_SET_KP:
                s_kp = msg.value;
                buzzerBeep(40);
                Serial.print(F("Kp -> "));
                Serial.println((int)s_kp);
                break;

            case CMD_SET_KI:
                s_ki = msg.value;
                buzzerBeep(40);
                Serial.print(F("Ki -> "));
                Serial.println((int)s_ki);
                break;

            case CMD_SET_KD:
                s_kd = msg.value;
                buzzerBeep(40);
                Serial.print(F("Kd -> "));
                Serial.println((int)s_kd);
                break;

            case CMD_RESET_PID:
                s_integral  = 0.0f;
                s_prevError = 0.0f;
                buzzerBeep(80);
                Serial.println(F("PID state reset"));
                break;

            case CMD_STATUS:
            {
                Serial.print(F("SP="));   Serial.print((int)s_setpoint);
                Serial.print(F(" Kp=")); Serial.print((int)s_kp);
                Serial.print(F(" Ki=")); Serial.print((int)s_ki);
                Serial.print(F(" Kd=")); Serial.println((int)s_kd);
                break;
            }

            default: break;
            }
        }

        // Read sensor data
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

        // PID calculation
        // error > 0 means temp < setpoint → need heating (output < 0 means heat)
        // We define: output > 0 = cooling needed, output < 0 = heating needed
        float error      = s_setpoint - temp;   // positive = too cold
        s_integral      += error * DT_S;

        // Anti-windup: clamp integral
        if (s_integral >  INTEGRAL_LIMIT) s_integral =  INTEGRAL_LIMIT;
        if (s_integral < -INTEGRAL_LIMIT) s_integral = -INTEGRAL_LIMIT;

        float derivative = (error - s_prevError) / DT_S;
        s_prevError = error;

        // Raw PID output: positive = need heating, negative = need cooling
        // We invert sign for actuator: positive output → fan speed
        float pidRaw = s_kp * error + s_ki * s_integral + s_kd * derivative;

        // Clamp
        if (pidRaw >  PID_OUT_MAX) pidRaw =  PID_OUT_MAX;
        if (pidRaw < -PID_OUT_MAX) pidRaw = -PID_OUT_MAX;

        // Actuator mapping:
        //   pidRaw > 0 → temp below setpoint → heater ON, fan OFF
        //   pidRaw < 0 → temp above setpoint → fan ON at |pidRaw|%, heater OFF
        //   pidRaw = 0 → both OFF
        bool    heaterOn = false;
        bool    fanOn    = false;
        uint8_t fanSpeed = 0;

        if (pidRaw > 0.0f)
        {
            heaterOn = true;
            relayOn();
            motorStop();
        }
        else if (pidRaw < 0.0f)
        {
            fanOn    = true;
            fanSpeed = (uint8_t)(-pidRaw);   // 0-100
            relayOff();
            motorSetPercent((float)fanSpeed);
        }
        else
        {
            relayOff();
            motorStop();
        }

        // LED indicators
        if (fanOn)
        {
            greenLed->turnOn();
            redLed->turnOff();
        }
        else if (heaterOn)
        {
            greenLed->turnOff();
            redLed->turnOn();
        }
        else
        {
            greenLed->turnOff();
            redLed->turnOff();
        }

        // Update control report
        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_controlRpt.setpoint  = s_setpoint;
        s_controlRpt.kp        = s_kp;
        s_controlRpt.ki        = s_ki;
        s_controlRpt.kd        = s_kd;
        s_controlRpt.error     = error;
        s_controlRpt.pidOutput = pidRaw;
        s_controlRpt.fanOn     = fanOn;
        s_controlRpt.fanSpeed  = fanSpeed;
        s_controlRpt.heaterOn  = heaterOn;
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
            // Page 0: temperature + fan/heater state
            int tInt  = (int)sr.temperature;
            int tFrac = ((int)(sr.temperature * 10.0f)) % 10;
            if (tFrac < 0) tFrac = -tFrac;

            lcdSetCursor(0, 0);
            printf("T:%d.%d SP:%dC  ", tInt, tFrac, (int)cr.setpoint);
            lcdSetCursor(0, 1);
            printf("Fan:%-3s Htr:%-3s  ",
                   cr.fanOn    ? "ON" : "OFF",
                   cr.heaterOn ? "ON" : "OFF");
        }
        else
        {
            // Page 1: PID gains
            lcdSetCursor(0, 0);
            printf("Kp:%d Ki:%d     ", (int)cr.kp, (int)cr.ki);
            lcdSetCursor(0, 1);
            printf("Kd:%d Out:%d%%   ", (int)cr.kd, (int)cr.pidOutput);
        }
        page ^= 1;

        // Teleplot serial output
        int tPlot  = (int)sr.temperature;
        int tFrac2 = ((int)(sr.temperature * 10.0f)) % 10;
        if (tFrac2 < 0) tFrac2 = -tFrac2;
        int errPlot = (int)cr.error;
        int outPlot = (int)cr.pidOutput;

        Serial.print(F(">Temp:"));
        Serial.print(tPlot); Serial.print('.'); Serial.print(tFrac2);
        Serial.print(F(",SetPoint:"));  Serial.print((int)cr.setpoint);
        Serial.print(F(",Error:"));     Serial.print(errPlot);
        Serial.print(F(",Output:"));    Serial.print(outPlot);
        Serial.print(F(",Fan:"));       Serial.print(cr.fanOn    ? 1 : 0);
        Serial.print(F(",Heater:"));    Serial.println(cr.heaterOn ? 1 : 0);

        vTaskDelay(pdMS_TO_TICKS(T3_PERIOD_MS));
    }
}

// Setup
void lab52Setup(void)
{
    Serial.begin(9600);
    lcdInit();

    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    buzzerInit();
    relayInit();
    motorInit();
    dhtInit();

    relayOn();          // heater ON by default at startup
    redLed->turnOn();

    // Init shared reports
    memset(&s_sensorRpt,  0, sizeof(s_sensorRpt));
    memset(&s_controlRpt, 0, sizeof(s_controlRpt));
    s_controlRpt.setpoint = DEFAULT_SETPOINT;
    s_controlRpt.kp       = DEFAULT_KP;
    s_controlRpt.ki       = DEFAULT_KI;
    s_controlRpt.kd       = DEFAULT_KD;

    s_reportMutex = xSemaphoreCreateMutex();
    s_cmdQueue    = xQueueCreate(4, sizeof(CmdMsg));

    xTaskCreate(taskAcquisition, "T1-Acq",  512, NULL, 2, NULL);
    xTaskCreate(taskController,  "T2-PID",  256, NULL, 3, NULL);
    xTaskCreate(taskDisplay,     "T3-Disp", 768, NULL, 1, NULL);

    lcdClear();
    printf("Lab 5.2 ready");
    Serial.println(F("Lab 5.2 - PID Temp Control"));
    Serial.println(F("Commands: set <C>  kp <v>  ki <v>  kd <v>  reset  status"));

    vTaskStartScheduler();
}

void lab52Loop(void)
{
    // Not reached - scheduler runs in lab52Setup()
}
