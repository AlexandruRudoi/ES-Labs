#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

/** Press is classified as short below this threshold (milliseconds). */
#define BUTTON_LONG_PRESS_MS 500UL

/** @brief Binds the driver to a pin. Call once before buttonUpdate().
 *  @param pin  Arduino pin the button is wired to (other leg → GND). */
void buttonInit(uint8_t pin);

/** @brief Samples the pin and advances the internal edge-detect state machine.
 *         Call periodically — every 20 ms provides inherent debouncing. */
void buttonUpdate(void);

/** @brief Returns true exactly once per completed press, then clears the flag.
 *  @return true if a new press event is waiting to be consumed. */
bool buttonWasPressed(void);

/** @brief Duration of the most recently completed press in milliseconds.
 *         Valid until the next completed press overwrites it. */
uint32_t buttonGetDuration(void);

/** @brief Returns true when the last completed press was shorter than BUTTON_LONG_PRESS_MS. */
bool buttonIsShortPress(void);

/** @brief Returns true when the last completed press was BUTTON_LONG_PRESS_MS or longer. */
bool buttonIsLongPress(void);

#endif
