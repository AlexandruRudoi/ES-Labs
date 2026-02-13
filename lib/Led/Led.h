#ifndef LED_H
#define LED_H

#include <Arduino.h>

class Led
{
public:
    Led(uint8_t pin);
    void turnOn();
    void turnOff();
    void toggle();
    bool getState() const;

private:
    uint8_t pin;
    bool state;
};

#endif