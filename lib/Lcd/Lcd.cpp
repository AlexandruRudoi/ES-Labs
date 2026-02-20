#include "Lcd.h"

static LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// track cursor position for STDIO output
static uint8_t cursorCol = 0;
static uint8_t cursorRow = 0;

static FILE lcdStream;

static int lcdPutc(char c, FILE *stream)
{
    if (c == '\n')
    {
        cursorRow++;
        cursorCol = 0;

        // wrap back to top if past last row
        if (cursorRow >= LCD_ROWS)
        {
            lcd.clear();
            cursorRow = 0;
        }

        lcd.setCursor(cursorCol, cursorRow);
    }
    else
    {
        lcd.print(c);
        cursorCol++;

        // wrap to next row if past last column
        if (cursorCol >= LCD_COLS)
        {
            cursorRow++;
            cursorCol = 0;

            if (cursorRow >= LCD_ROWS)
            {
                lcd.clear();
                cursorRow = 0;
            }

            lcd.setCursor(cursorCol, cursorRow);
        }
    }

    return 0;
}

void lcdInit()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // redirect stdout to LCD
    fdev_setup_stream(&lcdStream, lcdPutc, NULL, _FDEV_SETUP_WRITE);
    stdout = &lcdStream;
}

void lcdClear()
{
    lcd.clear();
    cursorCol = 0;
    cursorRow = 0;
    lcd.setCursor(0, 0);
}

void lcdSetCursor(uint8_t col, uint8_t row)
{
    cursorCol = col;
    cursorRow = row;
    lcd.setCursor(col, row);
}
