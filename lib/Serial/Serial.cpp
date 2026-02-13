#include "Serial.h"

// write char to serial
static int serialPutc(char c, FILE *stream)
{
    Serial.write(c);
    return 0;
}

// read char from serial
static int serialGetc(FILE *stream)
{
    while (Serial.available() == 0);
    return Serial.read();
}

static FILE serialStream;

void serialInit(unsigned long baudRate)
{
    Serial.begin(baudRate);

    // setup STDIO for read and write
    fdev_setup_stream(&serialStream, serialPutc, serialGetc, _FDEV_SETUP_RW);
    stdout = &serialStream;
    stdin = &serialStream;

    serialPrint("System Ready!");
    serialPrint("Send 'led on' or 'led off'");
}

bool serialReadCommand(char* buffer, size_t size)
{
    if (Serial.available() > 0)
    {
        // use scanf from STDIO library
        // read until newline or carriage return, max 19 chars
        if (scanf("%19[^\r\n]", buffer) == 1)
        {
            // clear input buffer
            while (getchar() != '\n');
            
            // echo back
            printf(">> %s\n", buffer);
            return true;
        }
    }
    return false;
}

void serialPrint(const char* message)
{
    printf("%s\n", message);
}