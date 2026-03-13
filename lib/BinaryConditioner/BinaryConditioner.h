/**
 * @file BinaryConditioner.h
 * @brief Binary signal conditioner: saturation -> hysteresis threshold -> debounce.
 *
 * Pipeline per channel:
 *   raw -> saturate([minVal,maxVal]) -> hysteresis(threshHigh/Low) -> debounce counter -> alert
 *
 *  Function          | Caller  | Notes
 *  ----------------- | ------- | ------------------------------------
 *  bcInit()          | setup   | reset state of one conditioner
 *  bcUpdate()        | Task 2  | feed new value, returns alert state
 *  bcIsAlert()       | Task 2  | current stable alert state
 *  bcIsNewAlert()    | Task 2  | true once on OK->ALERT edge
 *  bcGetSaturated()  | Task 3  | last clamped value (for display)
 */

#ifndef BINARY_CONDITIONER_H
#define BINARY_CONDITIONER_H

#include <Arduino.h>
#include <stdbool.h>

/** Configuration and runtime state for one conditioning channel.
 *  Set minVal/maxVal/threshHigh/threshLow/debounceMax, then call bcInit(). */
typedef struct
{
    /* --- configuration (set by caller) --------------------------------- */
    float    minVal;        /**< Saturation lower clamp.                   */
    float    maxVal;        /**< Saturation upper clamp.                   */
    float    threshHigh;    /**< Value above which alert is activated.      */
    float    threshLow;     /**< Value below which alert is cleared.        */
    uint8_t  debounceMax;   /**< Number of stable samples needed to switch. */

    /* --- internal state (owned by module) ------------------------------ */
    float    saturated;     /**< Last saturated value (for display).        */
    int8_t   counter;       /**< Up/down debounce counter [0, debounceMax]. */
    bool     alertState;    /**< Current stable output state.               */
    bool     newAlert;      /**< Set once on rising edge; cleared by read.  */
} BinaryConditioner;

/** @brief Reset counter and alert state. Call once after filling the struct. */
void bcInit(BinaryConditioner *bc);

/** @brief Process a new value through the full pipeline.
 *  @return Current stable alert state. */
bool bcUpdate(BinaryConditioner *bc, float value);

/** @brief Returns the current stable alert state. */
bool bcIsAlert(const BinaryConditioner *bc);

/** @brief Returns true once on OK->ALERT edge, then self-clears. */
bool bcIsNewAlert(BinaryConditioner *bc);

/** @brief Returns the last saturated (clamped) value. */
float bcGetSaturated(const BinaryConditioner *bc);

#endif
