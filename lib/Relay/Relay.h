/**
 * @file Relay.h
 * @brief Single-channel relay driver with active-LOW / active-HIGH support.
 *
 * Wraps a digital pin driving a relay module.  Set RELAY_ACTIVE_LOW to
 * match the hardware (most opto-isolated modules are active-LOW).
 *
 *  Function          | Notes
 *  ----------------- | -----------------------------------------------
 *  relayInit()       | call once in setup()
 *  relayOn()         | energise the coil (logical ON)
 *  relayOff()        | de-energise the coil (logical OFF)
 *  relayToggle()     | flip current state
 *  relayGetState()   | returns current logical state
 */

#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include <stdbool.h>

/** Arduino digital pin connected to the relay IN / signal pin. */
#define RELAY_PIN           3

/** Set to 1 for active-LOW relay modules, 0 for active-HIGH. */
#define RELAY_ACTIVE_LOW    1

/** @brief Initialises the relay pin and sets relay OFF.  Call once in setup(). */
void relayInit(void);

/** @brief Energises the relay coil (logical ON). */
void relayOn(void);

/** @brief De-energises the relay coil (logical OFF). */
void relayOff(void);

/** @brief Toggles the relay to the opposite state. */
void relayToggle(void);

/** @brief Returns the current logical state of the relay.
 *  @return true if ON, false if OFF. */
bool relayGetState(void);

#endif
