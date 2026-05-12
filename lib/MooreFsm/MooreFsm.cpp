#include "MooreFsm.h"

void fsmInit(MooreFsm *fsm, const MooreState *table, uint8_t numStates, uint8_t initialState)
{
    fsm->table     = table;
    fsm->numStates = numStates;
    fsm->current   = initialState;
}

void fsmEval(MooreFsm *fsm, uint8_t input)
{
    if (input > 2) input = 2;
    uint8_t next = fsm->table[fsm->current].next[input];
    if (next < fsm->numStates)
        fsm->current = next;
}

uint8_t fsmGetOutput(const MooreFsm *fsm)
{
    return fsm->table[fsm->current].out;
}

uint16_t fsmGetHoldMs(const MooreFsm *fsm)
{
    return fsm->table[fsm->current].holdMs;
}

uint8_t fsmGetState(const MooreFsm *fsm)
{
    return fsm->current;
}
