#include <iostream>

#include "FSM.h"



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