#ifndef LAB5_2_H
#define LAB5_2_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler.  This function never returns.
 *
 * Lab 5.2 — PID Temperature Control (Variant A).
 *   T1 : Sensor acquisition + command parser  (50 ms,  prio 2)
 *   T2 : PID controller                       (100 ms, prio 3)
 *   T3 : Display + serial plotter             (500 ms, prio 1)
 */
void lab52Setup(void);

/**
 * @brief Unused — vTaskStartScheduler() takes over in lab52Setup().
 */
void lab52Loop(void);

#endif
