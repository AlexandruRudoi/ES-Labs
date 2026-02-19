#ifndef LED_H
#define LED_H

#include <Arduino.h>

/** Driver for a single digital LED. */
class Led
{
public:
    /** @brief Initialises the LED bound to the given Arduino pin.
     *  @param pin  Arduino pin number the LED anode is connected to. */
    Led(uint8_t pin);

    /** @brief Turns the LED on (sets pin HIGH). */
    void turnOn();

    /** @brief Turns the LED off (sets pin LOW). */
    void turnOff();

    /** @brief Toggles the LED to the opposite state. */
    void toggle();

    /** @brief Returns the current logical state of the LED.
     *  @return true if on, false if off. */
    bool getState() const;

private:
    uint8_t pin;
    bool state;
};

#endif