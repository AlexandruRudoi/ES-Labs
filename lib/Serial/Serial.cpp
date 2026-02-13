#include "Serial.h"

// redirect printf to serial
static int serialPutc(char c, FILE *stream)
{
    Serial.write(c);
    return 0;
}

static FILE serialStdout;

void serialInit(unsigned long baudRate)
{
    Serial.begin(baudRate);

    // setup STDIO to use serial
    fdev_setup_stream(&serialStdout, serialPutc, NULL, _FDEV_SETUP_WRITE);
    stdout = &serialStdout;

    serialPrint("System Ready!");
    serialPrint("Send 'led on' or 'led off'");
}

bool serialReadCommand(char* buffer, size_t size)
{
    if (Serial.available() > 0)
    {
        size_t n = Serial.readBytesUntil('\n', buffer, size - 1);
        buffer[n] = '\0';

        // echo back
        printf(">> %s\n", buffer);

        // remove carriage return if present
        for (size_t i = 0; i < n; i++)
        {
            if (buffer[i] == '\r')
            {
                buffer[i] = '\0';
                break;
            }
        }
        return true;
    }
    return false;
}

void serialPrint(const char* message)
{
    printf("%s\n", message);
}