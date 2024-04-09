#include <Arduino.h>
#include <Arduino_CAN.h>
#include "FSM.h"
#include <map>

#define FAULT_PIN D3
#define NFAULT_PIN D2

extern State initialState;
extern std::map<FSM_STATE, State*> state_map;

State* currentState = &initialState;

void setup() {
    Serial.begin(9600);
    Serial1.setTimeout(1000);
    Serial.setTimeout(1000);

    pinMode(FAULT_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT);
    digitalWrite(FAULT_PIN, LOW);

    // bootCommands();
}

uint8_t loop_counter = 0;

void loop() {
    // Serial.println("On main");
    (*(currentState->action))();
    FSM_STATE nextState = (*(currentState->transition))();
    currentState = state_map[nextState];

    loop_counter++; // This will be used for tasks we decide are periodic such that we can run the state action items async from the update timer
    if(loop_counter == 10){
        loop_counter = 0;
    }
    delay(100);
}