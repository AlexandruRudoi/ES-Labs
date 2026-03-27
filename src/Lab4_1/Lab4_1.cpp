#include "Lab4_1.h"

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
#include <Button.h>

// Pin assignments
#define GREEN_LED_PIN   4   // relay ON indicator
#define RED_LED_PIN     5   // relay OFF indicator
#define BUTTON_PIN      2   // toggle button

// Task timing
#define TASK1_PERIOD_MS   50UL   // serial + button poll
#define TASK3_PERIOD_MS  500UL   // display refresh

// Output debounce
#define OD_MIN_HOLD_MS   500U    // min time between toggles
#define CMD_BUF_SIZE      16

typedef enum
{
    CMD_NONE = 0,
    CMD_ON,
    CMD_OFF,
    CMD_TOGGLE,
    CMD_STATUS
} CmdCode;

// T2 writes this, T3 reads it (mutex protected)
typedef struct
{
    bool     relayState;
    bool     lastCmdAccepted;
    CmdCode  lastCmd;
    uint32_t toggleCount;
    uint32_t rejectCount;
    uint32_t heldSinceMs;
} ActuatorReport;

static ActuatorReport     s_report;
static SemaphoreHandle_t  s_reportMutex = NULL;
static QueueHandle_t      s_cmdQueue    = NULL;

static Led *greenLed = NULL;
static Led *redLed   = NULL;

static OutputDebounce     s_od;



static char    s_lineBuf[CMD_BUF_SIZE];
static uint8_t s_lineIdx = 0;

// Accumulates serial chars without blocking; returns true when a line is ready
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

static CmdCode parseCommand(const char *cmd)
{
    if (strcmp(cmd, "on")     == 0) return CMD_ON;
    if (strcmp(cmd, "off")    == 0) return CMD_OFF;
    if (strcmp(cmd, "toggle") == 0) return CMD_TOGGLE;
    if (strcmp(cmd, "status") == 0) return CMD_STATUS;
    return CMD_NONE;
}

// T1 - reads serial + button, sends command to T2
static void taskCommandParser(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period    = pdMS_TO_TICKS(TASK1_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    for (;;)
    {
        // parse command once a full line has arrived
        if (serialLineReady())
        {
            // trim leading spaces + lowercase in-place
            uint8_t i = 0, j = 0;
            while (s_lineBuf[i] && isspace((unsigned char)s_lineBuf[i])) i++;
            while (s_lineBuf[i] && !isspace((unsigned char)s_lineBuf[i]))
                s_lineBuf[j++] = (char)tolower((unsigned char)s_lineBuf[i++]);
            s_lineBuf[j] = '\0';
            s_lineIdx = 0;

            CmdCode code = parseCommand(s_lineBuf);

            if (code != CMD_NONE)
            {
                xQueueSend(s_cmdQueue, &code, 0);
            }
            else
            {
                Serial.print(F("Unknown: "));
                Serial.println(s_lineBuf);
            }
        }

        buttonUpdate();
        if (buttonWasPressed())
        {
            CmdCode btn = CMD_TOGGLE;
            xQueueSend(s_cmdQueue, &btn, 0);
        }

        vTaskDelayUntil(&xLastWake, period);
    }
}

// T2 - applies debounce, drives relay + LEDs
static void taskConditionAndControl(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        CmdCode cmd;
        if (xQueueReceive(s_cmdQueue, &cmd, portMAX_DELAY) != pdTRUE)
            continue;
        bool accepted = true;

        switch (cmd)
        {
        case CMD_ON:
            accepted = odRequest(&s_od, true);
            if (accepted) relayOn();
            break;

        case CMD_OFF:
            accepted = odRequest(&s_od, false);
            if (accepted) relayOff();
            break;

        case CMD_TOGGLE:
            accepted = odRequest(&s_od, !relayGetState());
            if (accepted) relayToggle();
            break;

        case CMD_STATUS:
            break;

        default:
            break;
        }

        // buzzer feedback
        if (cmd != CMD_STATUS)
        {
            if (accepted)
            {
                buzzerBeep(80);
            }
            else
            {
                buzzerBeep(40);
                vTaskDelay(pdMS_TO_TICKS(60));
                buzzerBeep(40);
            }
        }

        // update LEDs
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
        s_report.relayState      = relayGetState();
        s_report.lastCmdAccepted = accepted;
        s_report.lastCmd         = cmd;
        s_report.toggleCount     = odGetToggleCount(&s_od);
        s_report.rejectCount     = odGetRejections(&s_od);
        s_report.heldSinceMs     = s_od.lastChangeMs;
        xSemaphoreGive(s_reportMutex);
    }
}

// T3 - LCD + serial output
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1000));

    for (;;)
    {
        ActuatorReport r;

        xSemaphoreTake(s_reportMutex, portMAX_DELAY);
        r = s_report;
        xSemaphoreGive(s_reportMutex);

        uint32_t heldMs  = millis() - r.heldSinceMs;
        uint16_t heldSec = (uint16_t)(heldMs / 1000UL);
        uint8_t  mm      = (uint8_t)(heldSec / 60);
        uint8_t  ss      = (uint8_t)(heldSec % 60);

        lcdSetCursor(0, 0);
        printf("Relay:%-3s %02u:%02u ",
               r.relayState ? "ON" : "OFF",
               mm, ss);

        const char *stat;
        if (r.lastCmd == CMD_NONE || r.lastCmd == CMD_STATUS)
            stat = "--";
        else
            stat = r.lastCmdAccepted ? "OK" : "RJ";

        lcdSetCursor(0, 1);
        printf("Tog:%-3lu Rej:%-3lu%2s",
               r.toggleCount, r.rejectCount, stat);

        // serial output — compact single line
        Serial.print(F("R:"));
        Serial.print(r.relayState ? 1 : 0);
        Serial.print(F(" T:"));
        Serial.print(r.toggleCount);
        Serial.print(F(" Rj:"));
        Serial.print(r.rejectCount);
        Serial.print(F(" H:"));
        Serial.print(heldSec);
        Serial.println(F("s"));

        vTaskDelay(pdMS_TO_TICKS(TASK3_PERIOD_MS));
    }
}

void lab41Setup(void)
{
    Serial.begin(9600);
    lcdInit();

    greenLed = new Led(GREEN_LED_PIN);
    redLed   = new Led(RED_LED_PIN);

    relayInit();
    buzzerInit();
    buttonInit(BUTTON_PIN);

    s_od.minHoldMs = OD_MIN_HOLD_MS;
    odInit(&s_od);

    redLed->turnOn();

    s_reportMutex = xSemaphoreCreateMutex();
    s_cmdQueue    = xQueueCreate(4, sizeof(CmdCode));

    s_report.relayState      = false;
    s_report.lastCmdAccepted = true;
    s_report.lastCmd         = CMD_NONE;
    s_report.toggleCount     = 0;
    s_report.rejectCount     = 0;
    s_report.heldSinceMs     = millis();

    BaseType_t r1 = xTaskCreate(taskCommandParser,       "T1-Cmd",  512, NULL, 2, NULL);
    BaseType_t r2 = xTaskCreate(taskConditionAndControl, "T2-Ctrl", 256, NULL, 3, NULL);
    BaseType_t r3 = xTaskCreate(taskDisplay,             "T3-Disp", 768, NULL, 1, NULL);
    (void)r1; (void)r2; (void)r3;

    lcdClear();
    printf("Lab 4.1 ready");
    Serial.println(F("Lab 4.1 - Binary Actuator Control"));
    Serial.println(F("Commands: on | off | toggle | status"));
    Serial.println(F("Button on pin 2 = toggle"));

    vTaskStartScheduler();
}

void lab41Loop(void)
{
    // Not reached — FreeRTOS scheduler runs in lab41Setup()
}
