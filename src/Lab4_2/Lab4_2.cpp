#include "Lab4_2.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>

#include <Lcd.h>
#include <Led.h>
#include <Relay.h>
#include <OutputDebounce.h>
#include <Buzzer.h>
#include <MotorDriver.h>
#include <MedianFilter.h>
#include <WeightedAverage.h>
#include <Ramp.h>

// Pin assignments
#define GREEN_LED_PIN   4
#define RED_LED_PIN     5

// Task periods
#define T1_PERIOD_MS    50UL
#define T3_PERIOD_MS    50UL
#define T4_PERIOD_MS   500UL

#define OD_MIN_HOLD_MS  500U    // relay min hold time

// Signal conditioning
#define MF_WINDOW       5       // median filter window
#define EMA_ALPHA       0.3f
#define RAMP_UP_RATE    50.0f   // %/s
#define RAMP_DOWN_RATE 100.0f   // %/s

#define CMD_BUF_SIZE    32

typedef enum
{
    CMD_NONE = 0,
    CMD_RELAY_ON,
    CMD_RELAY_OFF,
    CMD_RELAY_TOGGLE,
    CMD_STATUS
} RelayCmdCode;

// T2 writes, T4 reads
typedef struct
{
    bool          relayState;
    bool          lastCmdAccepted;
    RelayCmdCode  lastCmd;
    uint32_t      toggleCount;
    uint32_t      rejectCount;
    uint32_t      heldSinceMs;
} RelayReport;

// T3 writes, T4 reads
typedef struct
{
    float   rawSetpoint;   // 0-100 %
    float   filtered;      // after median + EMA
    float   ramped;        // after ramp
    float   pwmPercent;
} MotorReport;

// FreeRTOS queues and mutex
static QueueHandle_t      s_relayCmdQueue  = NULL;
static QueueHandle_t      s_speedCmdQueue  = NULL;
static SemaphoreHandle_t  s_reportMutex    = NULL;

static RelayReport  s_relayRpt;
static MotorReport  s_motorRpt;

static Led            *greenLed = NULL;
static Led            *redLed   = NULL;
static OutputDebounce  s_od;

// Signal conditioning pipeline state
static MedianFilter    s_mf;
static WeightedAverage s_wa;
static Ramp            s_ramp;
static float           s_rawSetpoint = 0.0f;

// Non-blocking line accumulator
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

// T1 - serial command parser (50 ms, prio 2)
// Uses sscanf on line buffer
static void taskCommandParser(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t period    = pdMS_TO_TICKS(T1_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    char cmd[CMD_BUF_SIZE];
    int  ival;

    for (;;)
    {
        if (serialLineReady())
        {
            s_lineIdx = 0;               // reset for next line

            // Debug: show what we received
            Serial.print(F("[DBG] buf='"));
            Serial.print(s_lineBuf);
            Serial.println(F("'"));

            // Step 1: read first word + track how many chars consumed
            int n = 0;
            if (sscanf(s_lineBuf, "%15s%n", cmd, &n) == 1)
            {
                // lowercase first word
                for (uint8_t i = 0; cmd[i]; i++)
                    cmd[i] = (char)tolower((unsigned char)cmd[i]);

                /* "relay" + sub-command */
                if (strcmp(cmd, "relay") == 0)
                {
                    char sub[CMD_BUF_SIZE];
                    if (sscanf(s_lineBuf + n, "%15s", sub) == 1)
                    {
                        for (uint8_t i = 0; sub[i]; i++)
                            sub[i] = (char)tolower((unsigned char)sub[i]);

                        RelayCmdCode code = CMD_NONE;
                        if      (strcmp(sub, "on")     == 0) code = CMD_RELAY_ON;
                        else if (strcmp(sub, "off")    == 0) code = CMD_RELAY_OFF;
                        else if (strcmp(sub, "toggle") == 0) code = CMD_RELAY_TOGGLE;

                        if (code != CMD_NONE)
                            xQueueSend(s_relayCmdQueue, &code, 0);
                        else
                        {
                            Serial.print(F("Bad relay cmd: "));
                            Serial.println(sub);
                        }
                    }
                    else
                        Serial.println(F("Usage: relay on|off|toggle"));
                }
                /* "speed" + number */
                else if (strcmp(cmd, "speed") == 0)
                {
                    if (sscanf(s_lineBuf + n, "%d", &ival) == 1)
                    {
                        float val = (float)ival;
                        xQueueSend(s_speedCmdQueue, &val, 0);
                    }
                    else
                        Serial.println(F("Usage: speed -100 to 100"));
                }
                /* single-word shortcuts */
                else if (strcmp(cmd, "stop") == 0)
                {
                    float zero = 0.0f;
                    xQueueSend(s_speedCmdQueue, &zero, 0);
                }
                else if (strcmp(cmd, "max") == 0)
                {
                    float full = 100.0f;
                    xQueueSend(s_speedCmdQueue, &full, 0);
                }
                else if (strcmp(cmd, "status") == 0)
                {
                    RelayCmdCode st = CMD_STATUS;
                    xQueueSend(s_relayCmdQueue, &st, 0);
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

// T2 - relay control (event-driven, prio 3)
static void taskRelayControl(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        RelayCmdCode cmd;
        if (xQueueReceive(s_relayCmdQueue, &cmd, portMAX_DELAY) != pdTRUE)
            continue;

        bool accepted = true;

        switch (cmd)
        {
        case CMD_RELAY_ON:
            accepted = odRequest(&s_od, true);
            if (accepted) relayOn();
            break;

        case CMD_RELAY_OFF:
            accepted = odRequest(&s_od, false);
            if (accepted) relayOff();
            break;

        case CMD_RELAY_TOGGLE:
            accepted = odRequest(&s_od, !relayGetState());
            if (accepted) relayToggle();
            break;

        case CMD_STATUS:
            break;

        default:
            break;
        }

        // Buzzer feedback
        if (cmd != CMD_STATUS)
        {
            if (accepted)
                buzzerBeep(80);
            else
            {
                buzzerBeep(40);
                vTaskDelay(pdMS_TO_TICKS(60));
                buzzerBeep(40);
            }
        }

        if (relayGetState())
        {
            greenLed->turnOn();
            redLed->turnOff();
        }
        else
        {
            greenLed->turnOff();
            redLed->turnOn();
        }

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_relayRpt.relayState      = relayGetState();
        s_relayRpt.lastCmdAccepted = accepted;
        s_relayRpt.lastCmd         = cmd;
        s_relayRpt.toggleCount     = odGetToggleCount(&s_od);
        s_relayRpt.rejectCount     = odGetRejections(&s_od);
        s_relayRpt.heldSinceMs     = s_od.lastChangeMs;
        xSemaphoreGive(s_reportMutex);
    }
}

// T3 - motor conditioning (50 ms, prio 3)
static void taskMotorCondition(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period    = pdMS_TO_TICKS(T3_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();
    const float      dt        = T3_PERIOD_MS / 1000.0f;

    for (;;)
    {
        float newSpeed;
        while (xQueueReceive(s_speedCmdQueue, &newSpeed, 0) == pdTRUE)
        {
            // Saturate -100 to 100
            if (newSpeed < -100.0f) newSpeed = -100.0f;
            if (newSpeed >  100.0f) newSpeed =  100.0f;
            s_rawSetpoint = newSpeed;
        }

        // Signal conditioning pipeline
        float med    = mfUpdate(&s_mf, s_rawSetpoint);
        float smooth = waUpdate(&s_wa, med);
        rampSetTarget(&s_ramp, smooth);
        float ramped = rampUpdate(&s_ramp, dt);

        motorSetPercentSigned(ramped);

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        s_motorRpt.rawSetpoint = s_rawSetpoint;
        s_motorRpt.filtered    = smooth;
        s_motorRpt.ramped      = ramped;
        s_motorRpt.pwmPercent  = motorGetPercentSigned();
        xSemaphoreGive(s_reportMutex);

        vTaskDelayUntil(&xLastWake, period);
    }
}

// T4 - LCD + serial display (500 ms, prio 1)
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1000));
    uint8_t page = 0;

    for (;;)
    {
        RelayReport rr;
        MotorReport mr;

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        rr = s_relayRpt;
        mr = s_motorRpt;
        xSemaphoreGive(s_reportMutex);

        if (page == 0)
        {
            // page 0: relay
            uint32_t heldMs  = millis() - rr.heldSinceMs;
            uint16_t heldSec = (uint16_t)(heldMs / 1000UL);
            uint8_t  mm      = (uint8_t)(heldSec / 60);
            uint8_t  ss      = (uint8_t)(heldSec % 60);

            const char *stat;
            if (rr.lastCmd == CMD_NONE || rr.lastCmd == CMD_STATUS)
                stat = "--";
            else
                stat = rr.lastCmdAccepted ? "OK" : "RJ";

            lcdSetCursor(0, 0);
            printf("Relay:%-3s %02u:%02u ",
                   rr.relayState ? "ON" : "OFF", mm, ss);
            lcdSetCursor(0, 1);
            printf("T:%-3lu R:%-3lu  %2s",
                   rr.toggleCount, rr.rejectCount, stat);
        }
        else
        {
            // page 1: motor
            lcdSetCursor(0, 0);
            printf("Set:%-3d Flt:%-3d.%d",
                   (int)mr.rawSetpoint,
                   (int)mr.filtered,
                   ((int)(mr.filtered * 10.0f)) % 10);
            lcdSetCursor(0, 1);
            printf("Rmp:%-3d.%d Pw:%-3d%%",
                   (int)mr.ramped,
                   ((int)(mr.ramped * 10.0f)) % 10,
                   (int)mr.pwmPercent);
        }
        page ^= 1;

        Serial.print(F("R:"));
        Serial.print(rr.relayState ? 1 : 0);
        Serial.print(F(" T:"));
        Serial.print(rr.toggleCount);
        Serial.print(F(" | Sp:"));
        Serial.print((int)mr.rawSetpoint);
        Serial.print(F(" Flt:"));
        Serial.print(mr.filtered, 1);
        Serial.print(F(" Rmp:"));
        Serial.print(mr.ramped, 1);
        Serial.print(F(" Pw:"));
        Serial.print((int)mr.pwmPercent);
        Serial.println(F("%"));

        vTaskDelay(pdMS_TO_TICKS(T4_PERIOD_MS));
    }
}

// Setup
void lab42Setup(void)
{
    Serial.begin(9600);
    lcdInit();

    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    relayInit();
    buzzerInit();
    motorInit();

    s_od.minHoldMs = OD_MIN_HOLD_MS;
    odInit(&s_od);

    s_mf.windowSize = MF_WINDOW;
    mfInit(&s_mf);

    s_wa.alpha = EMA_ALPHA;
    waInit(&s_wa);

    s_ramp.rateUp   = RAMP_UP_RATE;
    s_ramp.rateDown = RAMP_DOWN_RATE;
    rampInit(&s_ramp);

    redLed->turnOn();

    /* FreeRTOS objects */
    s_reportMutex   = xSemaphoreCreateMutex();
    s_relayCmdQueue = xQueueCreate(4, sizeof(RelayCmdCode));
    s_speedCmdQueue = xQueueCreate(4, sizeof(float));

    memset(&s_relayRpt, 0, sizeof(s_relayRpt));
    s_relayRpt.heldSinceMs = millis();
    memset(&s_motorRpt, 0, sizeof(s_motorRpt));

    /* Create tasks */
    xTaskCreate(taskCommandParser,  "T1-Cmd",  512, NULL, 2, NULL);
    xTaskCreate(taskRelayControl,   "T2-Rly",  256, NULL, 3, NULL);
    xTaskCreate(taskMotorCondition, "T3-Mtr",  256, NULL, 3, NULL);
    xTaskCreate(taskDisplay,        "T4-Disp", 768, NULL, 1, NULL);

    lcdClear();
    printf("Lab 4.2 ready");
    Serial.println(F("Lab 4.2 - Combined Actuator Control"));
    Serial.println(F("Commands: relay on|off|toggle  speed N  stop  max  status"));

    vTaskStartScheduler();
}

void lab42Loop(void)
{
    // Not reached - scheduler runs in lab42Setup()
}
