#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

/** Maximum number of tasks the scheduler can hold. */
#define SCHEDULER_MAX_TASKS 8

/** Plain task function — no parameters, no return value. */
typedef void (*TaskFunc)(void);

/**
 * @brief Descriptor for one scheduled task.
 *
 * Fill in func, recurrence, and offset.
 * The scheduler owns counter and ready — do not touch them.
 */
typedef struct
{
    TaskFunc         func;        /**< Function to call when the task is due.           */
    uint16_t         recurrence;  /**< Period between activations in milliseconds.       */
    uint16_t         offset;      /**< Initial delay before the first activation (ms).  */
    volatile int16_t counter;     /**< Countdown register — decremented by ISR each ms. */
    volatile bool    ready;       /**< Set by ISR when counter reaches 0.               */
} Task;

/** @brief Configures Timer2 for a 1 ms CTC interrupt and registers tasks.
 *  @param tasks  Array of Task descriptors (func/recurrence/offset filled by caller).
 *  @param count  Number of entries in the array. */
void schedulerInit(Task *tasks, uint8_t count);

/** @brief Executes every task whose ready flag is set, then clears the flag.
 *         Call as the only statement inside loop() — no blocking code allowed. */
void schedulerDispatch(void);

#endif
