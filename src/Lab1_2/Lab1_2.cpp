#include "Lab1_2.h"

#include <Arduino.h>
#include <string.h>
#include <Lcd.h>
#include <KeypadInput.h>
#include <Led.h>
#include <Debug.h>

#define GREEN_LED_PIN A3
#define RED_LED_PIN 3 // A4 and A5 are reserved for I2C (SDA/SCL)
#define CODE_LENGTH 4
#define RESULT_DELAY 2000

static const char secretCode[CODE_LENGTH + 1] = "1234";

static Led greenLed(GREEN_LED_PIN);
static Led redLed(RED_LED_PIN);

void lab12Setup()
{
    dbg.begin(9600);
    dbg.print("[DEBUG] Lab 1.2 starting...");

    lcdInit();
    dbg.print("[DEBUG] LCD initialized");

    keypadInit();
    dbg.print("[DEBUG] Keypad initialized");

    // LED self-test: flash both LEDs once
    dbg.print("[DEBUG] LED self-test: green ON");
    greenLed.turnOn();
    delay(500);
    greenLed.turnOff();

    dbg.print("[DEBUG] LED self-test: red ON");
    redLed.turnOn();
    delay(500);
    redLed.turnOff();

    dbg.print("[DEBUG] Setup done. Waiting for keypad input...");
}

void lab12Loop()
{
    char inputCode[CODE_LENGTH + 1];

    // prompt user
    lcdClear();
    printf("Enter code:");
    lcdSetCursor(0, 1);

    // read exactly CODE_LENGTH keys via STDIO (stdin = keypad), confirm with '#'
    scanf("%4s", inputCode);
    dbg.printVal("[DEBUG] Full code entered: ", inputCode);

    // validate code and show result on LCD
    lcdClear();
    if (strcmp(inputCode, secretCode) == 0)
    {
        dbg.print("[DEBUG] Code VALID -> green LED on");
        printf("Access Granted!");
        greenLed.turnOn();
        redLed.turnOff();
    }
    else
    {
        dbg.print("[DEBUG] Code INVALID -> red LED on");
        printf("Access Denied!");
        greenLed.turnOff();
        redLed.turnOn();
    }

    delay(RESULT_DELAY);
    greenLed.turnOff();
    redLed.turnOff();
}
