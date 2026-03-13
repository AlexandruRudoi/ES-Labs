#ifndef LAB3_1_H
#define LAB3_1_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler. This function never returns.
 */
void lab31Setup(void);

/**
 * @brief Unused - vTaskStartScheduler() takes over in lab31Setup().
 *        Provided for symmetry with other labs.
 */
void lab31Loop(void);

#endif
