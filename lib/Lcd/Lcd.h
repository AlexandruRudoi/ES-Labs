#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <stdio.h>
#include <LiquidCrystal_I2C.h>

#define LCD_COLS    16       /**< Number of character columns on the display. */
#define LCD_ROWS    2        /**< Number of character rows on the display. */
#define LCD_ADDRESS 0x27     /**< I2C address — try 0x3F if the display stays blank. */

/** @brief Initialises the I2C LCD and redirects stdout to the display.
 *         Call once in setup() before any printf(). */
void lcdInit();

/** @brief Clears all characters and moves the cursor to column 0, row 0. */
void lcdClear();

/** @brief Moves the cursor to the given position.
 *  @param col  Zero-based column index (0 – LCD_COLS-1).
 *  @param row  Zero-based row index (0 – LCD_ROWS-1). */
void lcdSetCursor(uint8_t col, uint8_t row);

#endif
