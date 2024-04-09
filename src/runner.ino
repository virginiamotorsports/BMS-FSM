#include <Arduino.h>
// #include <SoftwareSerial.h>
#include "FSM.h"
#include <map>

#define FAULT_PIN D3
#define NFAULT_PIN D2

extern State initialState;
extern std::map<FSM_STATE, State*> state_map;

State* currentState = &initialState;

void setup() {
    Serial.begin(9600);

    pinMode(FAULT_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT);
    digitalWrite(FAULT_PIN, LOW);

    Serial.println("On loop");

    // bootCommands();
}

void loop() {
    Serial.println("On main");
    (*(currentState->action))();
    FSM_STATE nextState = (*(currentState->transition))();
    currentState = state_map[nextState];
}