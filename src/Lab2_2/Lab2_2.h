#ifndef LAB2_2_H
#define LAB2_2_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 * the scheduler.  This function never returns.
 */
void lab22Setup(void);

/**
 * @brief Unused - vTaskStartScheduler() takes over the CPU inside lab22Setup().
 * Provided for symmetry with other labs; loop() in main.cpp calls it but it
 * will never be reached.
 */
void lab22Loop(void);

#endif