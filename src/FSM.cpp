#include <iostream>

#include "FSM.h"

/*
//
// Created by Miktad Cakmak on 1/26/24.
//

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
*/


int main(){
    State state;

    state.presentState = INITIAL;

    while(1){
        switch(state.presentState){
            case INITIAL:

                std::cout << "inside INITIAL" << std::endl;

                break;

            case STARTUP:

                break;

            case NORMALOP:

                break;

            case CELL_BALANCE:

                break;

            case FAULT_COMM:

                break;

            case FAULT_TMPVOLT:

                break;

            case FAULT_UNEXPECTED:

                break;


        }
    }
}