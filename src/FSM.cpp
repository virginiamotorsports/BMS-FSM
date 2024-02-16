#include <iostream>

#include "FSM.h"

State state;
bool communicationsOnOff = false;
bool fault = true;
bool readyToRun = false;
bool endCellBalancing = false;
bool startCellBalancing = false;

int establishConnection(){
    return 0;
}

void transition(FSM_STATE nextState) {
    state.presentState = nextState;
}

void initial() {
    //Run Boot commands

    //Establish communications
    bool connectionSuccess = establishConnection();
    if (connectionSuccess) {
        communicationsOnOff = 1;
    }

    //Exit Routine
    if (communicationsOnOff) {
        fault = 0; //Pull fault to low
        transition(STARTUP);
    }
}

void startup() {
    if (communicationsOnOff){
        //Write registers to the device

        //Exit Routine
        transition(NORMALOP);
    }
}

void normalOperations() {
    if (readyToRun){
        //Read voltages and temps

        //Send data via CAN

        fault = 1; //Fault pin goes high until fault

        //Exit Routine
        if(startCellBalancing){
            transition(CELL_BALANCE);
        }
    }
}

void unexpectedFault(){
    //Perform hardware reset on device

    //Send diagnostic CAN msg

    fault = 0; //fault pin = low;

    //Exit Routine
    //run the routine??
    transition(INITIAL);
}

bool commEstablished(){
    //Check if comms are established
    return false;
}

void commFault(){
    while(1){
        if(commEstablished()){
            break;
        }
    }
    fault = 0;
    transition(STARTUP);
}

void tempVoltageFault(){
    //Send CAN msgs and error state
    while(1){
        if(optimalValuesAchieved()){
            break;
        }
    }
    transition(NORMALOP);
}

bool optimalValuesAchieved() {
    return false;
}

void cellBalancing() {
    runCellBalancing();

    //Exit Routine
    if (endCellBalancing) {
        balancingComplete=false;
        transition(NORMALOP);
    }
    //Check faults
}

int main() {
    state.presentState = INITIAL;
    while (1) {
        switch (state.presentState) {
            case INITIAL:
                initial();
                break;
            default:
        }
    }
}

