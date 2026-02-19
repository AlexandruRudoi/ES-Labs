#include "Lab1_1.h"

#include <Arduino.h>
#include <string.h>
#include <Serial.h>
#include <Led.h>

#define LED_PIN        13
#define CMD_BUFFER_SIZE 20

static Led led(LED_PIN);

void lab11Setup()
{
    serialInit(9600);
}

void lab11Loop()
{
    char command[CMD_BUFFER_SIZE];

    if (serialReadCommand(command, sizeof(command)))
    {
        if (strcmp(command, "led on") == 0)
        {
            led.turnOn();
            serialPrint("LED is now ON");
        }
        else if (strcmp(command, "led off") == 0)
        {
            led.turnOff();
            serialPrint("LED is now OFF");
        }
        else
        {
            serialPrint("Invalid command! Use 'led on' or 'led off'");
        }
    }
}
