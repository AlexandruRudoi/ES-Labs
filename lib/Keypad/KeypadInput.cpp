#include "KeypadInput.h"

static char keyMap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

static byte rowPins[KEYPAD_ROWS] = {ROW_PIN_1, ROW_PIN_2, ROW_PIN_3, ROW_PIN_4};
static byte colPins[KEYPAD_COLS] = {COL_PIN_1, COL_PIN_2, COL_PIN_3, COL_PIN_4};

static Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

static FILE keypadStream;

// blocks until a key is pressed; maps '#' to '\n' so scanf can terminate,
// and echoes '*' to stdout (LCD) for every real key press
static int keypadGetc(FILE *stream)
{
    char key = NO_KEY;
    while (key == NO_KEY)
    {
        key = keypad.getKey();
    }
    if (key == '#')
    {
        return '\n';
    }
    putchar('*');
    return (int)key;
}

void keypadInit()
{
    // redirect stdin to keypad
    fdev_setup_stream(&keypadStream, NULL, keypadGetc, _FDEV_SETUP_READ);
    stdin = &keypadStream;
}

char keypadGetKey()
{
    return (char)getchar();
}
