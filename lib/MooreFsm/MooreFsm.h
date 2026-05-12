#ifndef MOORE_FSM_H
#define MOORE_FSM_H

#include <Arduino.h>

/**
 * @brief One state in a Moore FSM table.
 *
 * The table is stored in SRAM as a const array.
 *
 *   out    — output value emitted while in this state (0 or 1).
 *   holdMs — how long to remain in this state (used by the caller for delay).
 *   next   — next[input]: target state index for input = 0, 1, or 2.
 */
typedef struct
{
    uint8_t  out;
    uint16_t holdMs;
    uint8_t  next[3];
} MooreState;

/**
 * @brief FSM handle.  Bind once with fsmInit(), then call fsmEval() each cycle.
 */
typedef struct
{
    const MooreState *table;      // pointer to the application's state table
    uint8_t           numStates;  // number of entries in the table
    uint8_t           current;    // index of the current state
} MooreFsm;

/**
 * @brief Bind the handle to a state table and set the initial state.
 * @param fsm          Pointer to the FSM handle to initialise.
 * @param table        Pointer to the application state table (must outlive the handle).
 * @param numStates    Number of entries in the table.
 * @param initialState Index of the starting state.
 */
void fsmInit(MooreFsm *fsm, const MooreState *table, uint8_t numStates, uint8_t initialState);

/**
 * @brief Evaluate one FSM cycle: advance to the next state for the given input.
 * @param fsm   Handle previously initialised with fsmInit().
 * @param input Input value: 0 (none), 1 (short press), or 2 (long press).
 */
void fsmEval(MooreFsm *fsm, uint8_t input);

/** @brief Return the output value of the current state. */
uint8_t  fsmGetOutput(const MooreFsm *fsm);

/** @brief Return the hold time (ms) of the current state. */
uint16_t fsmGetHoldMs(const MooreFsm *fsm);

/** @brief Return the current state index. */
uint8_t  fsmGetState(const MooreFsm *fsm);

#endif
