/**
 * @file Ramp.h
 * @brief Linear ramp generator for smooth actuator transitions.
 *
 * Produces a linearly ramping output that moves from the current value
 * toward a target at a configurable rate (units per second).
 *
 *  Function           | Notes
 *  ------------------ | ------------------------------------------------
 *  rampInit()         | reset state, output = 0
 *  rampSetTarget()    | set the desired target value
 *  rampUpdate()       | advance ramp by dt seconds, returns current value
 *  rampGetValue()     | last computed output
 *  rampGetTarget()    | current target
 */

#ifndef RAMP_H
#define RAMP_H

#include <Arduino.h>

typedef struct
{
    /* --- configuration (set by caller) --------------------------------- */
    float rateUp;       /**< Max rise rate   (units / second). */
    float rateDown;     /**< Max fall rate   (units / second). */

    /* --- internal state ------------------------------------------------ */
    float target;       /**< Desired output value.             */
    float value;        /**< Current ramping output.           */
} Ramp;

/** @brief Reset ramp state. Call once after setting rateUp / rateDown. */
void  rampInit(Ramp *r);

/** @brief Set a new target value for the ramp to move toward. */
void  rampSetTarget(Ramp *r, float target);

/** @brief Advance the ramp by @p dt seconds. Returns current output. */
float rampUpdate(Ramp *r, float dt);

/** @brief Return last computed ramp output. */
float rampGetValue(const Ramp *r);

/** @brief Return current target. */
float rampGetTarget(const Ramp *r);

#endif
