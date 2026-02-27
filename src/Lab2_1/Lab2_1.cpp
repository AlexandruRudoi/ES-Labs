#include "Lab2_1.h"

#include <Arduino.h>
#include <stdio.h>
#include <Serial.h>
#include <Led.h>
#include <Button.h>
#include <Scheduler.h>
#include <Signals.h>

// Pin assignments
#define GREEN_LED_PIN   4
#define RED_LED_PIN     5
#define YELLOW_LED_PIN  6
#define BUTTON_PIN      7

// Timing / scheduling constants
#define TASK1_REC_MS         50UL
#define TASK2_REC_MS         50UL
#define TASK3_REC_MS         10000UL

#define LED_FEEDBACK_TICKS   10    // 10 * 50 ms = 500 ms feedback window
#define BLINK_HALF_TICKS     2     //  2 * 50 ms = 100 ms half-period
#define SHORT_PRESS_BLINKS   5     // visible blinks for a short press
#define LONG_PRESS_BLINKS    10    // visible blinks for a long press

// LED handles
static Led greenLed(GREEN_LED_PIN);
static Led redLed(RED_LED_PIN);
static Led yellowLed(YELLOW_LED_PIN);

/* 
   Task 1 — Button monitoring (recurrence: 50 ms, offset: 0 ms)
   PROVIDER: sig_pressEvent, sig_pressDuration, sig_pressIsShort

   Internal state (statics):
     feedbackTicks — counts down 50 ms ticks until green/red LED turns off
*/
static void task1ButtonMonitor(void)
{
    static int8_t feedbackTicks = 0;

    buttonUpdate();

    // turn off feedback LED after the window expires
    if (feedbackTicks > 0)
    {
        feedbackTicks--;
        if (feedbackTicks == 0)
        {
            greenLed.turnOff();
            redLed.turnOff();
        }
    }

    if (buttonWasPressed())
    {
        sig_pressDuration = buttonGetDuration();
        sig_pressIsShort  = buttonIsShortPress();
        sig_pressEvent    = true;   // consumed and cleared by Task 2

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

        feedbackTicks = LED_FEEDBACK_TICKS;
    }
}

/*
 Task 2 — Statistics update + yellow LED blink (recurrence: 50 ms, offset: 5 ms)
   CONSUMER of sig_pressEvent from Task 1
   PROVIDER of sig_stat* for Task 3

   Internal state (statics):
     blinkHalfCycles  — total half-cycles remaining (2 per visible blink)
     blinkHalfTicks   — tick countdown for each half-cycle
*/
static void task2StatsAndBlink(void)
{
    static int16_t blinkHalfCycles = 0;  // remaining half-cycles
    static int8_t  blinkHalfTicks  = 0;  // ticks left in current half-cycle

    // consume press event produced by Task 1
    if (sig_pressEvent)
    {
        sig_pressEvent = false;

        sig_statTotal++;
        if (sig_pressIsShort)
        {
            sig_statShort++;
            sig_statShortTotalMs += sig_pressDuration;
            blinkHalfCycles = SHORT_PRESS_BLINKS * 2;
        }
        else
        {
            sig_statLong++;
            sig_statLongTotalMs += sig_pressDuration;
            blinkHalfCycles = LONG_PRESS_BLINKS * 2;
        }

        blinkHalfTicks = BLINK_HALF_TICKS;
        // LED starts OFF — the state machine below drives every half-cycle.
        // With blinkHalfCycles = N*2 and LED starting OFF:
        //   toggle 1: ON  | toggle 2: OFF  | ... | toggle N*2: OFF  -> N clean blinks.
        yellowLed.turnOff();
    }

    // non-blocking blink tick: advance one half-period counter per task call
    if (blinkHalfCycles > 0)
    {
        blinkHalfTicks--;
        if (blinkHalfTicks <= 0)
        {
            blinkHalfTicks = BLINK_HALF_TICKS;

            yellowLed.toggle();
            blinkHalfCycles--;

            if (blinkHalfCycles == 0)
            {
                yellowLed.turnOff();   // ensure LED is off (last toggle may leave it on)
            }
        }
    }
}

/*
Task 3 — Periodic STDIO report (recurrence: 10 000 ms, offset: 10 ms)
   CONSUMER of sig_stat* accumulated by Task 2; resets counters after printing
*/
static void task3Report(void)
{
    uint32_t totalMs = sig_statShortTotalMs + sig_statLongTotalMs;
    uint32_t avgMs   = (sig_statTotal > 0) ? (totalMs / sig_statTotal) : 0;

    printf("\n=== Report (10s) ===\n");
    printf("Total presses : %lu\n",    sig_statTotal);
    printf("Short presses : %lu\n",    sig_statShort);
    printf("Long  presses : %lu\n",    sig_statLong);
    printf("Avg duration  : %lu ms\n", avgMs);
    printf("====================\n");

    // reset counters so the next 10 s interval starts from zero
    sig_statTotal        = 0;
    sig_statShort        = 0;
    sig_statLong         = 0;
    sig_statShortTotalMs = 0;
    sig_statLongTotalMs  = 0;
}

// Scheduler task table 
// Only func / recurrence / offset are set here.
// counter and ready are initialised to safe values by schedulerInit().
static Task s_tasks[] = {
    //  func                 recurrence (ms)   offset (ms)  counter  ready
    { task1ButtonMonitor,   TASK1_REC_MS,      0,           0,       false },
    { task2StatsAndBlink,   TASK2_REC_MS,      5,           0,       false },
    { task3Report,          TASK3_REC_MS,      10,          0,       false },
};

// Lab entry points
void lab21Setup(void)
{
    serialInit(9600);   // opens UART, redirects stdout / stdin

    buttonInit(BUTTON_PIN);

    schedulerInit(s_tasks, sizeof(s_tasks) / sizeof(s_tasks[0]));

    printf("Lab 2.1 ready. Press the button!\n");
    printf("T1: ButtonMonitor rec=%lums off=0ms\n",  TASK1_REC_MS);
    printf("T2: StatsAndBlink rec=%lums off=5ms\n",  TASK2_REC_MS);
    printf("T3: Report        rec=%lums off=10ms\n", TASK3_REC_MS);
}

void lab21Loop(void)
{
    schedulerDispatch();
}
