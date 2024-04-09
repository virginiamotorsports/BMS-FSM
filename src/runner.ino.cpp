# 1 "C:\\Users\\johnl\\AppData\\Local\\Temp\\tmp2uj5lest"
#include <Arduino.h>
# 1 "C:/Users/johnl/Documents/BMS-FSM/src/runner.ino"
#include <Arduino.h>
#include <Arduino_CAN.h>
#include "arduino_helpers.hpp"
#include "FSM.h"
#include <map>

extern State initialState;
extern std::map<FSM_STATE, State*> state_map;

State* currentState = &initialState;
void setup();
void loop();
#line 12 "C:/Users/johnl/Documents/BMS-FSM/src/runner.ino"
void setup() {



    pinMode(FAULT_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT);
    pinMode(IMD_STATUS, INPUT_PULLDOWN);
    pinMode(POS_AIR_STATUS, INPUT_PULLDOWN);
    pinMode(NEG_AIR_STATUS, INPUT_PULLDOWN);

    digitalWrite(FAULT_PIN, LOW);


}

uint8_t loop_counter = 0;

void loop() {

    (*(currentState->action))();
    FSM_STATE nextState = (*(currentState->transition))();
    currentState = state_map[nextState];




    loop_counter++;
    if(loop_counter == 10){
        loop_counter = 0;
    }
    delay(500);
}