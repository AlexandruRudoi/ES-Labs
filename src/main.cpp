#include <Arduino.h>
#include <string.h>

#include <Led.h>
#include <Serial.h>

#define LED_PIN 13
#define CMD_BUFFER_SIZE 20

Led led(LED_PIN);

void setup()
{
  serialInit(9600);
}

void loop()
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