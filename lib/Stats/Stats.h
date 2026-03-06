/**
 * @file Stats.h
 * @brief Mutex-protected press statistics module.
 *
 * Provides setter and getter functions for shared press statistics,
 * following the provider/consumer pattern with mutex-protected access.
 *
 *  Function           | Caller  | Mutex held
 *  ------------------ | ------- | ----------
 *  statsInit()        | setup   | creates mutex
 *  statsSetPress()    | Task 2  | yes
 *  statsGetAndReset() | Task 3  | yes
 */

#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialise the statistics module.
 * Creates the internal mutex. Must be called before tasks start.
 */
void statsInit(void);

/**
 * @brief Record one button press.
 * Acquires the mutex, increments the appropriate counters, releases the mutex.
 *
 * @param isShort  true = short press (< 500 ms), false = long press
 * @param durationMs  measured press duration in milliseconds
 */
void statsSetPress(bool isShort, uint32_t durationMs);

/**
 * @brief Read all statistics and atomically reset them to zero.
 * Acquires the mutex, copies values to output pointers, zeros all counters,
 * releases the mutex.
 *
 * @param[out] total     total number of presses since last report
 * @param[out] shortCnt  number of short presses
 * @param[out] longCnt   number of long presses
 * @param[out] avgMs     average press duration in ms (0 if no presses)
 */
void statsGetAndReset(uint32_t *total,
                      uint32_t *shortCnt,
                      uint32_t *longCnt,
                      uint32_t *avgMs);

#endif