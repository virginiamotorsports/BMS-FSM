#include <Arduino.h>
#include <Arduino_CAN.h>
#include "arduino_helpers.hpp"
#include "FSM.h"
#include <map>

extern State initialState;
extern std::map<FSM_STATE, State*> state_map;

State* currentState = &initialState;

void setup() {
    
    // establishConnection();

    pinMode(FAULT_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT);
    pinMode(IMD_STATUS, INPUT_PULLDOWN);
    pinMode(POS_AIR_STATUS, INPUT_PULLDOWN);
    pinMode(NEG_AIR_STATUS, INPUT_PULLDOWN);

    digitalWrite(FAULT_PIN, LOW);

    // bootCommands();
}

uint8_t loop_counter = 0;

void loop() {
    // Serial.println("On main");
    (*(currentState->action))();
    FSM_STATE nextState = (*(currentState->transition))();
    currentState = state_map[nextState];
    // Serial.print("nextState: ");
    // Serial.println(nextState);
    // Serial.println(uint32_t(&normalOpTransition));

    loop_counter++; // This will be used for tasks we decide are . such that we can run the state action items async from the update timer
    if(loop_counter == 10){
        loop_counter = 0;
    }
    delay(500);
}