/**
 * @file Buzzer.h
 * @brief Passive buzzer driver — single-tone blocking and non-blocking beep.
 *
 * Uses Arduino tone() / noTone() to drive a passive buzzer on a configurable pin.
 *
 *  Function          | Notes
 *  ----------------- | -----------------------------------------------
 *  buzzerInit()      | call once in setup()
 *  buzzerBeep()      | blocking: tone on for durationMs then off
 *  buzzerOn()        | non-blocking: start tone
 *  buzzerOff()       | non-blocking: stop tone
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

/** Arduino digital pin the passive buzzer positive leg is connected to. */
#define BUZZER_PIN          11

/** Default alert tone frequency in Hz. */
#define BUZZER_FREQ_HZ      1000U

/** @brief Initialises the buzzer pin. Call once in setup(). */
void buzzerInit(void);

/**
 * @brief Blocking beep — plays a tone for the given duration then stops.
 * @param durationMs  Duration in milliseconds.
 */
void buzzerBeep(uint16_t durationMs);

/** @brief Non-blocking: starts the alert tone continuously. */
void buzzerOn(void);

/** @brief Non-blocking: stops the tone. */
void buzzerOff(void);

#endif
