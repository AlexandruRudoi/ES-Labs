/**
 * @file OutputDebounce.h
 * @brief Output-side debouncer — enforces a minimum hold time between state changes.
 *
 * Prevents relay chatter by rejecting requests that arrive before the
 * configured minimum hold time has elapsed.
 *
 *  Function            | Notes
 *  ------------------- | -----------------------------------------------
 *  odInit()            | reset state; caller must set minHoldMs first
 *  odRequest()         | request a new state; returns true if accepted
 *  odGetState()        | current debounced output state
 *  odGetRejections()   | total rejected requests since init
 *  odGetToggleCount()  | total accepted toggles since init
 *  odGetHeldMs()       | time in current state (ms)
 */

#ifndef OUTPUT_DEBOUNCE_H
#define OUTPUT_DEBOUNCE_H

#include <Arduino.h>
#include <stdbool.h>

/** Configuration and runtime state for one output debouncer.
 *  Set minHoldMs, then call odInit(). */
typedef struct
{
    /* --- configuration (set by caller) --------------------------------- */
    uint16_t minHoldMs;     /**< Minimum hold time before a change is allowed. */

    /* --- internal state (owned by module) ------------------------------ */
    bool     state;         /**< Current debounced output state.                */
    uint32_t lastChangeMs;  /**< millis() timestamp of last accepted change.    */
    uint32_t toggleCount;   /**< Number of accepted toggles since init.         */
    uint32_t rejectCount;   /**< Number of rejected requests since init.        */
} OutputDebounce;

/** @brief Resets internal state. Caller must set minHoldMs first. */
void odInit(OutputDebounce *od);

/** @brief Requests a state change.  Rejected if the hold time hasn't elapsed.
 *  @param od          Pointer to the debouncer instance.
 *  @param requestedOn Desired logical state.
 *  @return true if the request was accepted. */
bool odRequest(OutputDebounce *od, bool requestedOn);

/** @brief Returns the current debounced output state. */
bool odGetState(const OutputDebounce *od);

/** @brief Returns the total number of rejected requests since init. */
uint32_t odGetRejections(const OutputDebounce *od);

/** @brief Returns the total number of accepted toggles since init. */
uint32_t odGetToggleCount(const OutputDebounce *od);

/** @brief Returns how long (ms) the output has been in its current state. */
uint32_t odGetHeldMs(const OutputDebounce *od);

#endif
