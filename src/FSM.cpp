#include <iostream>

#include "FSM.h"

State state;
int communicationsOnOff = 0;
int fault = 1;
int readyToRun = 0;


void transition(FSM_STATE nextState) {
    state.presentState = nextState;
}

void initial() {
    //Run Boot commands

    //Establish communications
    int connectionSuccess = establishConnection();
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
    if(communicationsOnOff){
        //Write registers to the device

        //Exit Routine
        transition(NORMALOP);
    }
}

void normalOperations() {
    if(readyToRun){
        //Read voltages and temps

        //Send data via CAN

        fault = 1; //Fault pin goes high until fault

        //Exit Routine

    }
}

void cellBalancing() {

}

int main() {
    state.presentState = INITIAL;
    while (1) {
        switch (state.presentState) {
            case INITIAL:
                break;
        }
    }
}