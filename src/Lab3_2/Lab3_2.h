#ifndef LAB3_2_H
#define LAB3_2_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler. This function never returns.
 */
void lab32Setup(void);

/**
 * @brief Unused - vTaskStartScheduler() takes over in lab32Setup().
 *        Provided for symmetry with other labs.
 */
void lab32Loop(void);

#endif