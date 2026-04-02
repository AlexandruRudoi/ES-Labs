#ifndef LAB4_1_H
#define LAB4_1_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler. This function never returns.
 */
void lab41Setup(void);

/**
 * @brief Unused - vTaskStartScheduler() takes over in lab41Setup().
 *        Provided for symmetry with other labs.
 */
void lab41Loop(void);

#endif
