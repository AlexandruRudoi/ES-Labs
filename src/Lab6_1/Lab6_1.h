#ifndef LAB6_1_H
#define LAB6_1_H

/**
 * @brief Initialise hardware, create FreeRTOS objects and tasks, then start
 *        the scheduler.  This function never returns.
 *
 * Lab 6.1 — Behavioural Control with Finite State Machines (Moore) – Button-LED.
 *
 *   FSM: 2-state Moore automaton
 *     State 0 (LED_OFF): output = 0, hold = 100 ms
 *     State 1 (LED_ON):  output = 1, hold = 100 ms
 *     Transition: toggle on every valid button press.
 *
 *   T1 (20 ms,  prio 3): button acquisition + debounce, signals T2 on press
 *   T2 (event,  prio 2): Moore FSM evaluation – applies output, transitions
 *   T3 (500 ms, prio 1): Serial display – state changes and heartbeat
 */
void lab61Setup(void);

/**
 * @brief Unused — vTaskStartScheduler() takes over in lab61Setup().
 *        Provided for symmetry with the other labs.
 */
void lab61Loop(void);

#endif
