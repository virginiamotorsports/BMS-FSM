#include <iostream>
#include <map>
#include "FSM.h"
#include "bq79616.hpp"
#include <math.h>
#include <stdio.h>
#include <WiFiS3.h>
#include <Arduino_CAN.h>
#include "can_helpers.hpp"
#include "BMSFSM.hpp"

#define FAULT_PIN D3
#define NFAULT_PIN D2

#define SECRET_SSID "BMS_WIFI"
#define SECRET_PASS "batteryboyz"

#define GET_TEMP(resistance) (1/(a1 + b1*(log(resistance/10000.0)) + c1*(pow(log(resistance/10000.0), 2)) + d1*(pow(log(resistance/10000.0), 3))) - 273.15)
#define GET_RESISTANCE(voltage) ((10000 * voltage) / (5 - voltage))

IPAddress send_to_address(192, 168, 244, 2);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;

const float_t a1 = 0.003354016;
const float_t b1 = 0.000256524;
const float_t c1 = 2.61E-6;
const float_t d1 = 6.33E-8;

unsigned long lastMillis;

char UDP_Buffer[200];
struct BMS_status modules[6];
int countTimer = 0;
bool comm_fault = false;
bool bms_fault = false;
bool n_fault = false;
bool OVUV_fault = false;
bool OTUT_fault = false;

uint16_t raw_data = 0;
int32_t signed_val = 0;
long double sr_val = 0;
char voltage_response_frame[(16 * 2 + 6) * TOTALBOARDS];    // hold all 16 vcell*_hi/lo values
char temp_response_frame[(8 * 2 + 6) * TOTALBOARDS];    // hold all 16 vcell*_hi/lo values
char response_frame_current[(1+6)]; //
WiFiUDP udp;

bool communicationsOnOff = false;
bool fault = true;
bool readyToRun = false;
bool endCellBalancing = false;
bool startCellBalancing = true;


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

bool establishConnection(){
    //Do we need wifi?
    //status = WiFi.beginAP(ssid, pass);
    //WiFi.config(IPAddress(192,168,244,1));

    return true;
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
    std::cout << "Initial!\n";

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

    return INITIAL;
}

void startupAction() {
    std::cout << "Startup!\n";
    if (communicationsOnOff){
        //Write registers to the device
    }
}

FSM_STATE startupTransition() {
    //Exit Routine
    if(startCellBalancing){
        return CELL_BALANCE;
    }
    return STARTUP;
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
    std::cout << "Cell balancing!\n";
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
    return CELL_BALANCE;
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
