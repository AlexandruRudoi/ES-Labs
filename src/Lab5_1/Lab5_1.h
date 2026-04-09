#ifndef LAB5_1_H
#define LAB5_1_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler.  This function never returns.
 *
 * Lab 5.1 — ON-OFF Temperature Control with Hysteresis (Variant A).
 *   T1 : Sensor acquisition      (50 ms,  prio 2)
 *   T2 : ON-OFF controller       (100 ms, prio 3)
 *   T3 : Display + serial plotter(500 ms, prio 1)
 */
void lab51Setup(void);

/**
 * @brief Unused — vTaskStartScheduler() takes over in lab51Setup().
 */
void lab51Loop(void);

#endif
