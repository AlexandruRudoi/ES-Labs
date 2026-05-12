#include "Lab6_1.h"

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include <Led.h>
#include <Button.h>
#include <Buzzer.h>
#include <MooreFsm.h>
#include <Lcd.h>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
#define LED_PIN    4   // controlled LED (active HIGH)
#define BUTTON_PIN 7   // toggle button (INPUT_PULLUP — other leg to GND)

// ---------------------------------------------------------------------------
// Task timing
// ---------------------------------------------------------------------------
#define T1_PERIOD_MS   20UL    // button poll / debounce interval
#define T3_PERIOD_MS  500UL    // display heartbeat period
#define BLINK_HALF_MS 250UL    // half-period of LED blink (250 ms ON, 250 ms OFF)

// ---------------------------------------------------------------------------
// Moore FSM definition  (lib/MooreFsm)
// ---------------------------------------------------------------------------
#define STATE_LED_OFF   0
#define STATE_LED_ON    1
#define STATE_LED_BLINK 2
#define FSM_NUM_STATES  3

// out=2 is the application-level sentinel for "blink" mode.
// Inputs: 0 = no press (timer tick),  1 = short press,  2 = long press.
//
//                                  out  hold    next[none]       next[short]      next[long]
static const MooreState FSM_TABLE[FSM_NUM_STATES] =
{
    /* STATE_LED_OFF   */ { 0, 100, { STATE_LED_OFF,   STATE_LED_ON,    STATE_LED_BLINK } },
    /* STATE_LED_ON    */ { 1, 100, { STATE_LED_ON,    STATE_LED_OFF,   STATE_LED_BLINK } },
    /* STATE_LED_BLINK */ { 2, 100, { STATE_LED_BLINK, STATE_LED_OFF,   STATE_LED_ON    } }
};

static const char * const STATE_NAMES[FSM_NUM_STATES] = { "LED_OFF", "LED_ON", "LED_BLINK" };

static MooreFsm s_fsm;

// ---------------------------------------------------------------------------
// Shared state and FreeRTOS objects
// ---------------------------------------------------------------------------
static QueueHandle_t     s_inputQueue = NULL;   // T1 -> T2: press type (1=short, 2=long)
static SemaphoreHandle_t s_dispSem    = NULL;   // T2 -> T3: state changed

static Led *led = NULL;   // heap-allocated so pinMode runs after Arduino init()

// ---------------------------------------------------------------------------
// T1 — Button acquisition (20 ms periodic, prio 3)
//
//   Calls buttonUpdate() to advance the debounce state machine.
//   Gives s_btnEvent once per validated press-release cycle.
// ---------------------------------------------------------------------------
static void taskAcquisition(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period    = pdMS_TO_TICKS(T1_PERIOD_MS);
    TickType_t       xLastWake = xTaskGetTickCount();

    for (;;)
    {
        buttonUpdate();

        if (buttonWasPressed())
        {
            uint8_t input = buttonIsLongPress() ? 2u : 1u;   // 1=short, 2=long
            xQueueSend(s_inputQueue, &input, 0);
        }

        vTaskDelayUntil(&xLastWake, period);
    }
}

// ---------------------------------------------------------------------------
// T2 — Moore FSM evaluation (event-driven + blink timer, prio 2)
//
//   Moore FSM cycle (per PDF Listing 7.5):
//     Step 1: output already applied from the previous cycle
//     Step 2: hold state for its defined time (debounce lockout)
//     Step 3: input = 1 (short press) or 2 (long press), from T1 queue
//     Step 4: transition to next state; apply new output
//
//   When in STATE_LED_BLINK the task uses a BLINK_HALF_MS queue timeout to
//   toggle the LED periodically while still accepting button events.
// ---------------------------------------------------------------------------
static void taskFSM(void *pvParameters)
{
    (void)pvParameters;

    // Apply initial output (LED_OFF)
    led->turnOff();

    for (;;)
    {
        // In BLINK state use a half-period timeout so we can toggle the LED;
        // otherwise block indefinitely until a button event arrives.
        TickType_t waitTicks = (fsmGetState(&s_fsm) == STATE_LED_BLINK)
                               ? pdMS_TO_TICKS(BLINK_HALF_MS)
                               : portMAX_DELAY;

        uint8_t input = 0;
        bool pressed = (xQueueReceive(s_inputQueue, &input, waitTicks) == pdTRUE);

        if (!pressed)
        {
            // Blink half-period elapsed — just toggle the LED and keep waiting
            led->toggle();
            continue;
        }

        // Step 2: hold window (debounce lockout)
        vTaskDelay(pdMS_TO_TICKS(fsmGetHoldMs(&s_fsm)));

        // Drain queue entries that piled up during the hold window
        while (xQueueReceive(s_inputQueue, &input, 0) == pdTRUE) {}

        // Steps 3-4: advance FSM and apply new output
        fsmEval(&s_fsm, input);

        uint8_t out = fsmGetOutput(&s_fsm);
        if (out == 0)
            led->turnOff();
        else if (out == 1)
            led->turnOn();
        // out == 2 → BLINK mode; LED toggling handled by the timeout branch above

        // Short beep on every transition
        buzzerBeep(80);

        // Notify T3 of the state change
        xSemaphoreGive(s_dispSem);
    }
}

// ---------------------------------------------------------------------------
// T3 — Serial display (prio 1)
//
//   Wakes immediately when s_dispSem is given (state change) to report the
//   new state.  Falls through on timeout to emit a periodic heartbeat so the
//   serial monitor always shows the current state even without button events.
// ---------------------------------------------------------------------------
static void taskDisplay(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        bool changed = (xSemaphoreTake(s_dispSem, pdMS_TO_TICKS(T3_PERIOD_MS)) == pdTRUE);
        uint8_t state = fsmGetState(&s_fsm);

        // Serial
        if (changed)
        {
            Serial.print(F("[FSM] -> "));
            Serial.println(STATE_NAMES[state]);
        }
        else
        {
            Serial.print(F("[FSM] State: "));
            Serial.println(STATE_NAMES[state]);
        }

        // LCD row 0: label, row 1: state name
        lcdSetCursor(0, 0);
        printf("%-16s", "Button-LED FSM");
        lcdSetCursor(0, 1);
        printf("%-16s", STATE_NAMES[state]);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void lab61Setup(void)
{
    Serial.begin(9600);
    lcdInit();    // redirects stdout to LCD

    led = new Led(LED_PIN);
    buttonInit(BUTTON_PIN);
    buzzerInit();
    fsmInit(&s_fsm, FSM_TABLE, FSM_NUM_STATES, STATE_LED_OFF);

    s_inputQueue = xQueueCreate(4, sizeof(uint8_t));
    s_dispSem    = xSemaphoreCreateBinary();

    xTaskCreate(taskAcquisition, "T1-Btn",  256, NULL, 3, NULL);
    xTaskCreate(taskFSM,         "T2-FSM",  256, NULL, 2, NULL);
    xTaskCreate(taskDisplay,     "T3-Disp", 256, NULL, 1, NULL);

    lcdClear();
    lcdSetCursor(0, 0);
    printf("%-16s", "Button-LED FSM");
    lcdSetCursor(0, 1);
    printf("%-16s", STATE_NAMES[STATE_LED_OFF]);

    Serial.println(F("Lab 6.1 — Button-LED Moore FSM (3-state)"));
    Serial.println(F("Short press: OFF<->ON | Long press: any->BLINK / BLINK->ON"));

    vTaskStartScheduler();
}

void lab61Loop(void)
{
    // Unreachable: vTaskStartScheduler() called in lab61Setup() never returns
}
