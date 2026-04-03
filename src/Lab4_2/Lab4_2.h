#ifndef LAB4_2_H
#define LAB4_2_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler.  This function never returns.
 *
 * Lab 4.2 — Combined Binary + Analog Actuator Control (Variant C).
 *   T1 : Serial command parser   (50 ms, prio 2)
 *   T2 : Binary relay control    (event-driven, prio 3)
 *   T3 : Analog motor conditioning (50 ms, prio 3)
 *   T4 : LCD + serial display    (500 ms, prio 1)
 */
void lab42Setup(void);

/**
 * @brief Unused — vTaskStartScheduler() takes over in lab42Setup().
 */
void lab42Loop(void);

#endif
