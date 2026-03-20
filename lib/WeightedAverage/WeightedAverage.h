/**
 * @file WeightedAverage.h
 * @brief Exponential moving average (EMA) filter for signal smoothing.
 *
 * Applies the formula:
 *   output = alpha * newValue + (1 - alpha) * previousOutput
 *
 * where alpha (0.0 – 1.0) controls responsiveness:
 *   alpha close to 1.0 → fast response, less smoothing
 *   alpha close to 0.0 → slow response, heavy smoothing
 *
 *  Function              | Caller  | Notes
 *  --------------------- | ------- | ------------------------------------
 *  waInit()              | setup   | reset state
 *  waUpdate()            | Task 2  | feed new sample, returns smoothed value
 *  waGetValue()          | Task 3  | last smoothed output
 */

#ifndef WEIGHTED_AVERAGE_H
#define WEIGHTED_AVERAGE_H

#include <Arduino.h>
#include <stdbool.h>

/** Configuration and runtime state for one EMA channel. */
typedef struct
{
    /* --- configuration (set by caller) --------------------------------- */
    float    alpha;         /**< Smoothing factor (0.0 – 1.0).             */

    /* --- internal state (owned by module) ------------------------------ */
    float    value;         /**< Current smoothed output.                  */
    bool     primed;        /**< False until first sample received.        */
} WeightedAverage;

/** @brief Reset state. Call once after setting alpha. */
void waInit(WeightedAverage *wa);

/** @brief Feed a new sample. Returns the smoothed output.
 *  On the very first call, output = input (no history to blend). */
float waUpdate(WeightedAverage *wa, float value);

/** @brief Returns the last smoothed output value. */
float waGetValue(const WeightedAverage *wa);

#endif
