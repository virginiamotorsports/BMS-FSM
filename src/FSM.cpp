#include "FSM.h"
#include "can_helpers.hpp"
#include "arduino_helpers.hpp"


extern uint8_t loop_counter;

unsigned long lastMillis = millis();
unsigned long temp_fault_timer = millis();
unsigned long voltage_fault_timer = millis();
unsigned long n_fault_timer = millis();

uint8_t message_data[LAST_ITEM - STATUS + 3][8]; //defined can msgs and the 

char UDP_Buffer[200];
BMS_status modules[] = {{}, {}, {}, {}, {}, {}};
int countTimer = 0;
bool comm_fault = false;
bool bms_fault = false;
bool n_fault = false;
bool OVUV_fault = false;
bool OTUT_fault = false;
bool ams_fault = false, imd_fault = false; // These are the latching faults
uint16_t cell_temp;
float_t pack_voltage = 0;
uint16_t pack_voltage_scaled = 0;
uint8_t soc = 0;
uint16_t cell_temps = 0;
uint32_t min_max_cell_volts = 0;
uint8_t fault_status = 0;

int32_t signed_val = 0;
long double sr_val = 0;
char voltage_response_frame[(16 * 2 + 6) * STACK_DEVICES];  // hold all 16 vcell*_hi/lo values
char temp_response_frame[(8 * 2 + 6) * STACK_DEVICES];  // hold all 16 vcell*_hi/lo values
char response_frame_current[(1 + 6)];   //

bool communicationsOnOff = false;
uint8_t pin_status, old_pin_status = 0;
int fault_pin = true;
bool readyToRun = false;
bool endCellBalancing = false;
bool startCellBalancing = true;
bool startup_done = false;

State initialState = { .action = &initialAction,
                      .transition = &initialTransition };
State startupState = { .action = &startupAction,
                      .transition = &startupTransition };
State normalOpState = { .action = &normalOpAction,
                       .transition = &normalOpTransition };
State cellBalancingState = { .action = &cellBalancingAction,
                            .transition = &cellBalancingTransition };
State commFaultState = { .action = &commFaultAction,
                        .transition = &commFaultTransition };
State tempVoltageFaultState = { .action = &tempVoltageFaultAction,
                               .transition = &tempVoltageFaultTransition };
State unexpectedFaultState = { .action = &unexpectedFaultAction,
                              .transition = &unexpectedFaultTransition };

std::map<FSM_STATE, State*> state_map = {
    {INITIAL, &initialState},
    {STARTUP, &startupState},
    {NORMALOP, &normalOpState},
    {CELL_BALANCE, &cellBalancingState},
    {FAULT_COMM, &commFaultState},
    {FAULT_TMPVOLT, &tempVoltageFaultState},
    {FAULT_UNEXPECTED, &unexpectedFaultState} };

uint16_t send_can_data(void){

    // Copy cell temps over
    memcpy(message_data[CELL_TEMP_0 - START_ITEM], modules[0].cell_temps, 8);
    memcpy(message_data[CELL_TEMP_1 - START_ITEM], modules[1].cell_temps, 8);
    memcpy(message_data[CELL_TEMP_2 - START_ITEM], modules[2].cell_temps, 8);
    memcpy(message_data[CELL_TEMP_3 - START_ITEM], modules[3].cell_temps, 8);
    memcpy(message_data[CELL_TEMP_4 - START_ITEM], modules[4].cell_temps, 8);
    memcpy(message_data[CELL_TEMP_5 - START_ITEM], modules[5].cell_temps, 8);

    // Copy cell voltages over
    memcpy(message_data[CELL_VOLTAGE_0 - START_ITEM], modules[0].cell_voltages, 8);
    memcpy(message_data[CELL_VOLTAGE_1 - START_ITEM], (modules[0].cell_voltages + 8), 8);

    memcpy(message_data[CELL_VOLTAGE_2 - START_ITEM], (modules[1].cell_voltages), 8);
    memcpy(message_data[CELL_VOLTAGE_3 - START_ITEM], (modules[1].cell_voltages + 8), 8);
    
    memcpy(message_data[CELL_VOLTAGE_4 - START_ITEM], modules[2].cell_voltages, 8);
    memcpy(message_data[CELL_VOLTAGE_5 - START_ITEM], (modules[2].cell_voltages + 8), 8);

    memcpy(message_data[CELL_VOLTAGE_6 - START_ITEM], (modules[3].cell_voltages), 8);
    memcpy(message_data[CELL_VOLTAGE_7 - START_ITEM], (modules[3].cell_voltages + 8), 8);
    
    memcpy(message_data[CELL_VOLTAGE_8 - START_ITEM], modules[4].cell_voltages, 8);
    memcpy(message_data[CELL_VOLTAGE_9 - START_ITEM], (modules[4].cell_voltages + 8), 8);

    memcpy(message_data[CELL_VOLTAGE_10 - START_ITEM], (modules[5].cell_voltages), 8);
    memcpy(message_data[CELL_VOLTAGE_11 - START_ITEM], (modules[5].cell_voltages + 8), 8);

    memcpy(message_data[LAST_ITEM + 1] + 2, &pack_voltage_scaled, 2);
    memcpy(message_data[LAST_ITEM + 1] + 4, &soc, 1);
    memcpy(message_data[LAST_ITEM + 2] + 5, &cell_temps, 2);
    memcpy(message_data[LAST_ITEM + 3] + 1, &min_max_cell_volts, 4);
    memcpy(message_data[STATUS - START_ITEM], &fault_status, 1);
    memcpy(message_data[STATUS - START_ITEM] + 1, &soc, 1);
    memcpy(message_data[STATUS - START_ITEM] + 2, &pack_voltage_scaled, 2);
    memcpy(message_data[STATUS - START_ITEM] + 4, &cell_temps, 2);



    for(uint32_t addr = START_ITEM; addr < LAST_ITEM; addr++){
        // memcpy((void *)(msg_data), &message_data[addr], sizeof(message_data[addr]));
        // Serial.println(addr);
        // *msg = CanMsg(0x400, sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]);

        int ret = CAN.write(CanMsg(CanStandardId(addr), sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]));
        if(!(ret == 0 || ret == 1)){
            if(DEBUG){
                Serial.print("CAN Error: ");
                Serial.println(ret);
            }
            CAN.clearError();
            break;
        }
        delay(10);
        // Serial.println(ret);
    }

    for(uint32_t addr = ORION_MSG_1; addr <= ORION_MSG_3; addr++){
        int ret = CAN.write(CanMsg(CanStandardId(addr), sizeof(message_data[LAST_ITEM + (addr - ORION_MSG_1) + 1]), message_data[LAST_ITEM + (addr - ORION_MSG_1) + 1]));
        if(!(ret == 0 || ret == 1)){
            if(DEBUG){
                Serial.print("CAN Error: ");
                Serial.println(ret);
            }
            CAN.clearError();
            break;
        }
        delay(10);
    }
    return cell_temps;
}


void printResponseFrameForDebug() {
    Serial.print("PROT: ");
    Serial.print((response_frame_current[4] & 0x80));

    Serial.print("\tADC: ");
    Serial.print((response_frame_current[4] & 0x40));

    Serial.print("\tOTP: ");
    Serial.print((response_frame_current[4] & 0x20));

    Serial.print("\tCOMM: ");
    Serial.print((response_frame_current[4] & 0x10));

    Serial.print("\tOTUT: ");
    Serial.print((response_frame_current[4] & 0x08));

    Serial.print("\tOVUV: ");
    Serial.print((response_frame_current[4] & 0x04));

    Serial.print("\tSYS: ");
    Serial.print((response_frame_current[4] & 0x02));

    Serial.print("\tPWR: ");
    Serial.println((response_frame_current[4] & 0x01));
}

bool establishConnection() {
    if(DEBUG){
        Serial.begin(9600);
        Serial.setTimeout(500);
    }
    else{
        Serial.begin(115200);
        Serial.setTimeout(500);
    }
    while(!Serial) {};
    Serial1.begin(BAUDRATE, SERIAL_8N1);
    Serial1.setTimeout(1000);
    while(!Serial1) {};
    CAN.begin(CanBitRate::BR_500k);

    return true;
}

bool commEstablished() {
    // Check if comms are established
    // Maybe send / receive some data to check if comms are established
    if(DEBUG){
        Serial.print("Hello this the the BMS Code\r\n");
    }
    return false;
}

void bootCommands() {
    // Run Boot commands

    Wake79616();
    delayMicroseconds((10000 + 520) * TOTALBOARDS);  // 2.2ms from shutdown/POR to active mode + 520us till
    // device can send wake tone, PER DEVICE

    WakeStack();
    delayMicroseconds((10000 + 520) * TOTALBOARDS); // 2.2ms from shutdown/POR to active mode + 520us till device can send wake tone, PER DEVICE
    if(DEBUG){
        Serial.println("Stack is Woken up\r\n");
    }

    AutoAddress();
    if(DEBUG){
        Serial.println("Autoaddress Completed");
    }

    set_registers();

    // clear the write buffer
    memset(UDP_Buffer, 0, sizeof(UDP_Buffer));
    memset(response_frame_current, 0, sizeof(response_frame_current));
    memset(message_data, 0, sizeof(message_data[0][0]) * message_data_width * 8);
    memset(temp_response_frame, 0, sizeof(temp_response_frame));
    memset(voltage_response_frame, 0, sizeof(voltage_response_frame));

    startup_done = true;
}

bool optimalValuesAchieved() { return false; }

void runCellBalancing() {
    // Run cell balancing
    // First Check CB state, if not cb, start, otherwise check
    if(pin_status & 0x2){
        if(DEBUG){
            Serial.println("Starting CB");
        }
        WriteReg(0, BAL_CTRL2, 0x33, 1, FRMWRT_STK_W);  // starts balancing all cells
        endCellBalancing = true;
    }
    else{
        endCellBalancing = true;
    }
    lastMillis = millis();

}

void initialAction() {
    // if(DEBUG){
    //     Serial.println(comm_fault);
    // }

    // Establish communications
    if (establishConnection()) {
        communicationsOnOff = true;
    }
}

FSM_STATE initialTransition() {
    // Exit Routine
    if (communicationsOnOff) {
        return STARTUP;
    }
    return INITIAL;
}

void startupAction() {
    if (communicationsOnOff) {
        // Write registers to the device
        bootCommands();
        delay(500);
        startup_done = true;
    }
}

FSM_STATE startupTransition() {
    // Exit Routine
    if (startup_done) {
        readyToRun = true;
        return NORMALOP;
    }
    else
        return STARTUP;
}

void normalOpAction() {
    if (readyToRun) {

        WriteReg(0, OVUV_CTRL, 0x05, 1, FRMWRT_STK_W);  // run OV UV checks
        WriteReg(0, OTUT_CTRL, 0x05, 1, FRMWRT_STK_W);  // run OT UT checks

        //printResponseFrameForDebug();

        read_faults(modules);


        if (OVUV_fault) {
            WriteReg(0, FAULT_RST1, 0x18, 1, FRMWRT_STK_W);  // reset fault
        }

        if (OTUT_fault) {
            WriteReg(0, FAULT_RST1, 0x60, 1, FRMWRT_STK_W);  // reset fault
        }

        OVUV_fault = false;
        OTUT_fault = false;

        for (int i = 0; i < STACK_DEVICES; i++) {
            if (modules[i].faults & 0x04) {
                OVUV_fault = true;
            }
            if (modules[i].faults & 0x08) {
                OTUT_fault = true;
            }
        }

        memset(&pin_status, 0, 1);
        pin_status = read_signal_pins(); // bits 0 -> nfault, 1 -> IMD_STATUS, 2 -> POS_AIR, 3 -> NEG_AIR, 4 -> RESET

        if(((old_pin_status >> 4) & 0x1 == 0x0) && ((pin_status >> 4) & 0x1)){
            ams_fault = false;
            imd_fault = false;
        }

        if((pin_status >> 1) & 0x1 == 0x0){
            imd_fault == true;
        }


        read_cell_voltages(modules);
        read_cell_temps(modules);
        // read_die_temps(modules);

        pack_voltage = sum_voltages(modules);
        pack_voltage_scaled = floor(pack_voltage * 10);
        soc = calc_soc(pack_voltage_scaled) * 2;
        cell_temps = calc_min_max_temp(modules);
        min_max_cell_volts = calc_min_max_volts(modules);
        fault_status = ((ams_fault & 0x1) << 1) | (imd_fault & 0x1);

        send_can_data();

        if((cell_temp >> 8) > 40 && (cell_temp >> 8) < 120){
            digitalWrite(FAN_PIN, HIGH); // Fan control code, turn on the fan if too hot, off if too cold but make sure there is some overlap to prevent oscilations 
        }
        else if((cell_temp >> 8) < 37){
            digitalWrite(FAN_PIN, LOW);
        }

        if(DEBUG){
            printBatteryCellVoltages(modules);
            printBatteryCellTemps(modules);
        }
    }
}

FSM_STATE normalOpTransition() {

    if (comm_fault) {
        if(DEBUG){
            Serial.println("COMM Fault");
        }
        digitalWrite(AMS_FAULT_PIN, LOW);
        ams_fault = true;
        return FAULT_COMM;
    }
    else if(OTUT_fault &&  millis() - temp_fault_timer > 5 * 1000UL){
        if(DEBUG){
            Serial.println("OTUT Fault");
        }
        digitalWrite(AMS_FAULT_PIN, LOW);
        ams_fault = true;
        return NORMALOP;
    }
    else if(OVUV_fault &&  millis() - voltage_fault_timer > 5 * 1000UL){
        if(DEBUG){
            Serial.println("OVUV Fault");
        }
        digitalWrite(AMS_FAULT_PIN, LOW);
        ams_fault = true;
        return NORMALOP;
    }
    // else if(n_fault &&  millis() - n_fault_timer > 5 * 1000UL){
    //     if(DEBUG){
    //         Serial.println("N Fault");
    //     }
    //     digitalWrite(AMS_FAULT_PIN, LOW);
    //     ams_fault = true;
    //     return NORMALOP;
    // }
    else if(millis() - lastMillis >= 30 * 1000UL){
            return CELL_BALANCE;
    }
    else {
        digitalWrite(AMS_FAULT_PIN, HIGH);
        n_fault_timer = millis();
        if(!OTUT_fault){
            temp_fault_timer = millis();
        }
        if(!OVUV_fault){
        voltage_fault_timer = millis(); //reset fault timers if there are no faults
        }
        return NORMALOP;
    }
}

void unexpectedFaultAction() {
    // Perform hardware reset on device

    // Send diagnostic CAN msg

    fault_pin = 0;  // fault_pin pin = low;
}

FSM_STATE unexpectedFaultTransition() { return INITIAL; }

void commFaultAction() {
    if(DEBUG){
        // Serial.println(comm_fault);
        // Serial.println("Comm fault action");
        Serial.println("Communications Fault");
    }
    delay(1000);
    bootCommands();
    fault_pin = 0;
}

FSM_STATE commFaultTransition() {
    if (comm_fault) {
        return FAULT_COMM;
    }
    return STARTUP;
}

void tempVoltageFaultAction() {
    // Send CAN msgs and error state
    while (true) {
        if (optimalValuesAchieved()) {
            break;
        }
    }
}

FSM_STATE tempVoltageFaultTransition() {
    // TODO: Stub
    return NORMALOP;
}

void cellBalancingAction() { runCellBalancing(); }

FSM_STATE cellBalancingTransition() {
    // TODO: Stub
    // Exit Routine
    if (endCellBalancing) {
        endCellBalancing = false;
        return NORMALOP;
    }
    // Check faults
    return CELL_BALANCE;
}