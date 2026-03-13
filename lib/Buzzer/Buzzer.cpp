#include "Buzzer.h"

void buzzerInit(void)
{
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
}

void buzzerBeep(uint16_t durationMs)
{
    tone(BUZZER_PIN, BUZZER_FREQ_HZ, durationMs);
    delay(durationMs);
    noTone(BUZZER_PIN);
}

void buzzerOn(void)
{
    tone(BUZZER_PIN, BUZZER_FREQ_HZ);
}

void buzzerOff(void)
{
    noTone(BUZZER_PIN);
}
