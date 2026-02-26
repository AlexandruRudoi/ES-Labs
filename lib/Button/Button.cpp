#include "Button.h"

static uint8_t  s_pin        = 0;
static bool     s_lastState  = true;   // HIGH = released (INPUT_PULLUP)
static bool     s_eventReady = false;
static uint32_t s_pressStart = 0;
static uint32_t s_duration   = 0;

void buttonInit(uint8_t pin)
{
    s_pin       = pin;
    s_lastState = HIGH;
    pinMode(pin, INPUT_PULLUP);
}

void buttonUpdate(void)
{
    bool current = (bool)digitalRead(s_pin);

    // falling edge — button pressed (INPUT_PULLUP: pressed = LOW)
    if (s_lastState == HIGH && current == LOW)
    {
        s_pressStart = millis();
    }
    // rising edge — button released
    else if (s_lastState == LOW && current == HIGH)
    {
        s_duration   = millis() - s_pressStart;
        s_eventReady = true;
    }

    s_lastState = current;
}

bool buttonWasPressed(void)
{
    if (s_eventReady)
    {
        s_eventReady = false;
        return true;
    }
    return false;
}

uint32_t buttonGetDuration(void)
{
    return s_duration;
}

bool buttonIsShortPress(void)
{
    return s_duration < BUTTON_LONG_PRESS_MS;
}

bool buttonIsLongPress(void)
{
    return s_duration >= BUTTON_LONG_PRESS_MS;
}
