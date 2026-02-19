#ifndef KEYPAD_INPUT_H
#define KEYPAD_INPUT_H

#include <Arduino.h>
#include <stdio.h>
#include <Keypad.h>

#define KEYPAD_ROWS  4   /**< Number of rows on the keypad matrix. */
#define KEYPAD_COLS  4   /**< Number of columns on the keypad matrix. */

/** Row pin assignments (Arduino digital pins). */
#define ROW_PIN_1    8
#define ROW_PIN_2    9
#define ROW_PIN_3    10
#define ROW_PIN_4    11

/** Column pin assignments (Arduino digital/analog pins). */
#define COL_PIN_1    12
#define COL_PIN_2    A0
#define COL_PIN_3    A1
#define COL_PIN_4    A2

/** @brief Initialises the keypad and redirects stdin so getchar() returns key presses.
 *         Call once in setup() before any getchar() or scanf(). */
void keypadInit();

/** @brief Blocks until a key is pressed and returns its character.
 *  @return Character from the keymap ('0'-'9', 'A'-'D', '*', '#'). */
char keypadGetKey();

#endif
