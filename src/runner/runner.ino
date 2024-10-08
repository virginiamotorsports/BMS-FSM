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

    pinMode(AMS_FAULT_PIN, OUTPUT);
    pinMode(NFAULT_PIN, INPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLDOWN);
    pinMode(IMD_STATUS, INPUT_PULLDOWN);
    pinMode(POS_AIR_STATUS, INPUT_PULLDOWN);
    pinMode(NEG_AIR_STATUS, INPUT_PULLDOWN);

    digitalWrite(AMS_FAULT_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);

    //CAN.setMailboxMask(4, 0x1FFFFFFFU); //disables reading can std msgs
    //CAN.setMailboxMask(6, 0x1FFFFFFFU); //disables reading can ext msgs see https://forum.arduino.cc/t/uno-r4-can-mask-filter/1177947/4


    
    // bootCommands();
}

uint8_t loop_counter = 0;

void loop() {
    // Serial.println("loop");
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
    delay(300);
}