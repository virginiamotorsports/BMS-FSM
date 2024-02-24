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
    void (*enter)();
    void (*action)();
    void (*exit)();
} State;

bool optimalValuesAchieved();
int establishConnection();
bool commEstablished();
bool optimalValuesAchieved();
void runCellBalancing();
void transition(FSM_STATE nextState);

void initial();
void startup();
void normalOperations();
void cellBalancing();
void commFault();
void tempVoltageFault();
void unexpectedFault();

#endif FSM_H