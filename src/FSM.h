#ifndef FSM_H
#define FSM_H

typedef enum{
    INITIAL,
    STARTUP,
    NORMALOP,
    CELL_BALANCE,
    FAULT_COMM,
    FAULT_TMPVOLT,
    FAULT_UNEXPECTED,
}FSM_STATE;

typedef struct {
    FSM_STATE presentState;
}State;

#endif FSM_H