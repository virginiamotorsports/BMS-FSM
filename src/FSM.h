#ifndef FSM_H
#define FSM_H

#include "bq79616.hpp"
// #include "can_helpers.hpp"

#include <Arduino.h>
#include <Arduino_CAN.h>
#include <math.h>
#include <stdio.h>

#include <map>


typedef enum{
    INITIAL,
    STARTUP,
    NORMALOP,
    CELL_BALANCE,
    FAULT_COMM,
    FAULT_TMPVOLT,
    FAULT_UNEXPECTED,
} FSM_STATE;

typedef struct {
    void (*action)();
    FSM_STATE (*transition)();
} State;

void bootCommands();
bool establishConnection();
bool commEstablished();
bool optimalValuesAchieved();
void runCellBalancing();
uint16_t send_can_data();

void initialAction();
FSM_STATE initialTransition();
void startupAction();
FSM_STATE startupTransition();
void normalOpAction();
FSM_STATE normalOpTransition();
void cellBalancingAction();
FSM_STATE cellBalancingTransition();
void commFaultAction();
FSM_STATE commFaultTransition();
void tempVoltageFaultAction();
FSM_STATE tempVoltageFaultTransition();
void unexpectedFaultAction();
FSM_STATE unexpectedFaultTransition();

#endif