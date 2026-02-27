/**
 * @file Signals.h
 * @brief Shared volatile signals for the provider / consumer model.
 *
 * All variables are declared volatile because they are written by one task
 * and read by another inside the non-preemptive dispatch loop, and the Timer2
 * ISR runs concurrently with the main loop.
 *
 *  Signal              | Type           | Provider | Consumer(s)
 *  ------------------  | -------------- | -------- | -----------
 *  sig_pressEvent      | volatile bool  | Task 1   | Task 2
 *  sig_pressDuration   | volatile u32   | Task 1   | Task 2
 *  sig_pressIsShort    | volatile bool  | Task 1   | Task 2
 *  sig_statTotal       | volatile u32   | Task 2   | Task 3
 *  sig_statShort       | volatile u32   | Task 2   | Task 3
 *  sig_statLong        | volatile u32   | Task 2   | Task 3
 *  sig_statShortTotalMs| volatile u32   | Task 2   | Task 3
 *  sig_statLongTotalMs | volatile u32   | Task 2   | Task 3
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdint.h>
#include <stdbool.h>

/* Task 1 -> Task 2 */

/** Set by Task 1 when a button press completes. Consumed and cleared by Task 2. */
extern volatile bool     sig_pressEvent;

/** Duration of the most recent press in ms. Written by Task 1, read by Task 2. */
extern volatile uint32_t sig_pressDuration;

/** true = short press (<500 ms), false = long press. Written by T1, read by T2. */
extern volatile bool     sig_pressIsShort;

/* Task 2 -> Task 3 */

/** Cumulative count of all button presses since last report. */
extern volatile uint32_t sig_statTotal;

/** Number of short presses since last report. */
extern volatile uint32_t sig_statShort;

/** Number of long presses since last report. */
extern volatile uint32_t sig_statLong;

/** Total cumulative duration (ms) of all short presses since last report. */
extern volatile uint32_t sig_statShortTotalMs;

/** Total cumulative duration (ms) of all long presses since last report. */
extern volatile uint32_t sig_statLongTotalMs;

#endif
