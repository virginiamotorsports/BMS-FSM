#include <iostream>
#include <map>

#include "FSM.h"

bool communicationsOnOff = false;
bool fault = true;
bool readyToRun = false;
bool endCellBalancing = false;
bool startCellBalancing = false;


State initialState = {.action=&initialAction, .transition=&initialTransition};
State startupState = {.action=&startupAction, .transition=&startupTransition};
State normalOpState = {.action=&normalOpAction, .transition=&normalOpTransition};
State cellBalancingState = {.action=&cellBalancingAction, .transition=&cellBalancingTransition};
State commFaultState = {.action=&commFaultAction, .transition=&commFaultTransition};
State tempVoltageFaultState = {.action=&tempVoltageFaultAction, .transition=&tempVoltageFaultTransition};
State unexpectedFaultState = {.action=&unexpectedFaultAction, .transition=&unexpectedFaultTransition};

std::map<FSM_STATE , State*> map = {{INITIAL, &initialState},
                                    {STARTUP, &startupState},
                                    {NORMALOP, &normalOpState},
                                    {CELL_BALANCE, &cellBalancingState},
                                    {FAULT_COMM, &commFaultState},
                                    {FAULT_TMPVOLT, &tempVoltageFaultState},
                                    {FAULT_UNEXPECTED, &unexpectedFaultState}};

int establishConnection(){
    return 0;
}

bool commEstablished(){
    //Check if comms are established
    return false;
}

void bootCommands(){
    //Run Boot commands

}

bool optimalValuesAchieved() {
    return false;
}

void runCellBalancing() {
    //Run cell balancing
}

void initialAction() {
    //Establish communications
    bool connectionSuccess = establishConnection();
    if (connectionSuccess) {
        communicationsOnOff = true;
    }
}

FSM_STATE initialTransition(){
    //Exit Routine
    if (communicationsOnOff) {
        fault = false; //Pull fault to low
        return STARTUP;
    }

    std::cout << "Hello, World!\n";

    return INITIAL;
}

void startupAction() {
    if (communicationsOnOff){
        //Write registers to the device
    }
}

FSM_STATE startupTransition() {
    //Exit Routine
    if(startCellBalancing){
        return CELL_BALANCE;
    }
}

void normalOpAction() {
    if (readyToRun){
        //Read voltages and temps

        //Send data via CAN

        fault = true; //Fault pin goes high until fault
    }
}

FSM_STATE normalOpTransition() {
    //TODO: Stub
}

void unexpectedFaultAction() {
    //Perform hardware reset on device

    //Send diagnostic CAN msg

    fault = false; //fault pin = low;
}

FSM_STATE unexpectedFaultTransition() {
    //TODO: Stub
    return INITIAL;
}

void commFaultAction(){
    while(true){
        if(commEstablished()){
            break;
        }
    }
    fault = 0;
}

FSM_STATE commFaultTransition() {
    //TODO: Stub
    return STARTUP;
}

void tempVoltageFaultAction(){
    //Send CAN msgs and error state
    while(true){
        if(optimalValuesAchieved()){
            break;
        }
    }
}

FSM_STATE tempVoltageFaultTransition() {
    //TODO: Stub
    return NORMALOP;
}

void cellBalancingAction() {
    runCellBalancing();
}

FSM_STATE cellBalancingTransition() {
    //TODO: Stub
    //Exit Routine
    if (endCellBalancing) {
        endCellBalancing=false;
        return NORMALOP;
    }
    //Check faults
}

int main() {
    State *currentState;
    currentState = &initialState;
    bootCommands();
    while(true) {
        (*(currentState->action))();
        FSM_STATE nextState = (*(currentState->transition))();
        currentState = map[nextState];
    }
}
