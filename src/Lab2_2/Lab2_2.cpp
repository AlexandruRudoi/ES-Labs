#include "Lab2_2.h"

#include <Arduino.h>
#include <stdio.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include <Serial.h>
#include <Button.h>
#include <Led.h>
#include <Signals.h>
#include <Stats.h>

// Pin assignments
#define GREEN_LED_PIN 4
#define RED_LED_PIN 5
#define YELLOW_LED_PIN 6
#define BUTTON_PIN 7

// Timing constants
#define TASK1_PERIOD_MS 50UL
#define TASK3_PERIOD_MS 10000UL

#define LED_FEEDBACK_MS 500UL // green / red on-time after press
#define BLINK_HALF_MS 100UL   // yellow half-period (on or off)
#define SHORT_PRESS_BLINKS 5  // number of visible blinks for short press
#define LONG_PRESS_BLINKS 10  // number of visible blinks for long press

// Synchronisation handles
// xPressEvent: binary semaphore — Task 1 gives, Task 2 takes.
// Replaces the volatile sig_pressEvent flag from Lab 2.1.
static SemaphoreHandle_t xPressEvent = NULL;

// LED handles
static Led greenLed(GREEN_LED_PIN);
static Led redLed(RED_LED_PIN);
static Led yellowLed(YELLOW_LED_PIN);

/*
   Task 1 — Button monitoring (sequential, vTaskDelayUntil 50 ms)
   PROVIDER:  writes sig_pressDuration, sig_pressIsShort then gives xPressEvent.
   All writes happen BEFORE xSemaphoreGive so Task 2 reads a consistent snapshot.
*/
static void taskButtonMonitor(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period = pdMS_TO_TICKS(TASK1_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // offset = 0 ms for Task 1 (first to run, no delay needed)

    for (;;)
    {
        buttonUpdate();

        if (buttonWasPressed())
        {
            // write signals before giving semaphore (safe: T2 is blocked until give)
            sig_pressDuration = buttonGetDuration();
            sig_pressIsShort = buttonIsShortPress();

            // visual feedback — LED on; Task 1 turns it off after LED_FEEDBACK_MS
            if (sig_pressIsShort)
            {
                greenLed.turnOn();
                redLed.turnOff();
            }
            else
            {
                redLed.turnOn();
                greenLed.turnOff();
            }

            // schedule LED off after feedback window
            vTaskDelay(pdMS_TO_TICKS(LED_FEEDBACK_MS));
            greenLed.turnOff();
            redLed.turnOff();

            // signal Task 2
            xSemaphoreGive(xPressEvent);
        }

        vTaskDelayUntil(&xLastWakeTime, period);
    }
}

/*
   Task 2 — Statistics update + yellow LED blink   (event-driven)
   CONSUMER of xPressEvent from Task 1.
   PRODUCER: calls statsSetPress() (mutex-protected setter).
   Blocks on xSemaphoreTake(portMAX_DELAY) — wakes only when T1 signals.
   Uses vTaskDelay inside blink loop — simple and readable, no state machine
   needed because FreeRTOS yields the CPU during each delay.
*/
static void taskStatsAndBlink(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        // block indefinitely until Task 1 signals a press event
        if (xSemaphoreTake(xPressEvent, portMAX_DELAY) == pdTRUE)
        {
            // read snapshot written by Task 1 before the Give
            bool isShort = sig_pressIsShort;
            uint32_t duration = sig_pressDuration;

            // update statistics under mutex
            statsSetPress(isShort, duration);

            // yellow LED blink sequence
            int blinks = isShort ? SHORT_PRESS_BLINKS : LONG_PRESS_BLINKS;
            yellowLed.turnOff();
            for (int i = 0; i < blinks * 2; i++)
            {
                yellowLed.toggle();
                vTaskDelay(pdMS_TO_TICKS(BLINK_HALF_MS));
            }
            yellowLed.turnOff();
        }
    }
}

/*
   Task 3 — Periodic report (sequential, vTaskDelayUntil 10 000 ms)
   CONSUMER: calls statsGetAndReset() — reads + zeros counters atomically.
   Reports via printf (STDIO redirected to UART by serialInit).
*/
static void taskReport(void *pvParameters)
{
    (void)pvParameters;

    // offset = 10 ms — ensures T1 fires at least once before first report
    vTaskDelay(pdMS_TO_TICKS(10));

    const TickType_t period = pdMS_TO_TICKS(TASK3_PERIOD_MS);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        uint32_t total, shortCnt, longCnt, avgMs;
        statsGetAndReset(&total, &shortCnt, &longCnt, &avgMs);

        printf("\n=== Report (10s) ===\n");
        printf("Total presses : %lu\n", total);
        printf("Short presses : %lu\n", shortCnt);
        printf("Long  presses : %lu\n", longCnt);
        printf("Avg duration  : %lu ms\n", avgMs);
        printf("====================\n");

        vTaskDelayUntil(&xLastWakeTime, period);
    }
}

void lab22Setup(void)
{
    serialInit(9600);

    buttonInit(BUTTON_PIN);

    // create mutex inside Stats module and binary semaphore for press events
    statsInit();
    xPressEvent = xSemaphoreCreateBinary();

    // create tasks - all priority 1 (equal) to avoid unwanted preemption
    xTaskCreate(taskButtonMonitor, "T1-Btn", 256, NULL, 1, NULL);
    xTaskCreate(taskStatsAndBlink, "T2-Stats", 256, NULL, 1, NULL);
    xTaskCreate(taskReport, "T3-Rep", 512, NULL, 1, NULL);

    printf("Lab 2.2 ready (FreeRTOS). Press the button!\n");
    printf("T1: ButtonMonitor  period=%lums\n", TASK1_PERIOD_MS);
    printf("T2: StatsAndBlink  event-driven\n");
    printf("T3: Report         period=%lums\n", TASK3_PERIOD_MS);

    // start the RTOS scheduler - never returns
    vTaskStartScheduler();
}

void lab22Loop(void)
{
    // unreachable: vTaskStartScheduler() called in lab22Setup() never returns
}
