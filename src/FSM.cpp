#include "FSM.h"
#include "can_helpers.hpp"
#include "arduino_helpers.hpp"

extern uint8_t loop_counter;

unsigned long lastMillis = millis();
uint8_t message_data[LAST_ITEM - STATUS][8];

char UDP_Buffer[200];
BMS_status modules[] = {{}, {}, {}, {}, {}, {}};
int countTimer = 0;
bool comm_fault = false;
bool bms_fault = false;
bool n_fault = false;
bool OVUV_fault = false;
bool OTUT_fault = false;

int32_t signed_val = 0;
long double sr_val = 0;
// char voltage_response_frame[(16 * 2 + 6) *
// TOTALBOARDS];  // hold all 16 vcell*_hi/lo values
// char temp_response_frame[(8 * 2 + 6) *
// TOTALBOARDS];  // hold all 16 vcell*_hi/lo values
char response_frame_current[(1 + 6)];   //

bool communicationsOnOff = false;
uint8_t pin_status;
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

void send_can_data(void){

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


  for(uint32_t addr = START_ITEM; addr < LAST_ITEM; addr++){
    // memcpy((void *)(msg_data), &message_data[addr], sizeof(message_data[addr]));
    // Serial.println(addr);
    // *msg = CanMsg(0x400, sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]);

    int ret = CAN.write(CanMsg(CanStandardId(addr), sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]));
    if(ret != 0){
      Serial.print("CAN Error: ");
      Serial.println(ret);
      CAN.clearError();
      break;
    }
    // Serial.println(ret);
  }
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
    Serial.begin(9600);
    Serial.setTimeout(500);
    Serial1.begin(BAUDRATE, SERIAL_8N1);
    Serial1.setTimeout(1000);
    CAN.begin(CanBitRate::BR_500k);

    return true;
}

bool commEstablished() {
    // Check if comms are established
    // Maybe send / receive some data to check if comms are established
    Serial.print("Hello this the the BMS Code\r\n");
    return false;
}

void bootCommands() {
    // Run Boot commands

    Wake79616();
    delayMicroseconds((10000 + 520) * TOTALBOARDS);  // 2.2ms from shutdown/POR to active mode + 520us till
    // device can send wake tone, PER DEVICE

    WakeStack();
    delayMicroseconds((10000 + 520) * TOTALBOARDS); // 2.2ms from shutdown/POR to active mode + 520us till device can send wake tone, PER DEVICE
    Serial.print("Stack is Woken up\r\n");

    AutoAddress();
    Serial.println("Autoaddress Completed");

    set_registers();

    // clear the write buffer
    memset(UDP_Buffer, 0, sizeof(UDP_Buffer));
    memset(response_frame_current, 0, sizeof(response_frame_current));
    memset(message_data, 0, sizeof(message_data[0][0]) * message_data_width * 8);

    startup_done = true;
}

bool optimalValuesAchieved() { return false; }

void runCellBalancing() {
    // Run cell balancing
    Serial.println("Running CB");
    WriteReg(0, BAL_CTRL2, 0x33, 1,
             FRMWRT_ALL_W);  // starts balancing all cells
}

void initialAction() {
    Serial.println(comm_fault);

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

        // printResponseFrameForDebug();

        read_faults(modules);

        OVUV_fault = false;
        OTUT_fault = false;

        for (int i = 0; i < STACK_DEVICES; i++) {
            if (modules[i].faults & 0x01 || modules[i].faults & 0x02) {
                OVUV_fault = true;
            }
            if (modules[i].faults & 0x03 || modules[i].faults & 0x04) {
                OTUT_fault = true;
            }
        }

        pin_status = read_signal_pins();

        read_cell_voltages(modules);
        read_cell_temps(modules);
        read_die_temps(modules);

        send_can_data();

        if (OVUV_fault) {
            WriteReg(0, FAULT_RST1, 0x18, 1, FRMWRT_STK_W);  // reset fault
        }

        if (OTUT_fault) {
            WriteReg(0, FAULT_RST1, 0x60, 1, FRMWRT_STK_W);  // reset fault
        }

        if (millis() - lastMillis >= 30 * 1000UL && !(OVUV_fault || OTUT_fault)) {
            lastMillis = millis();  // get ready for the next iteration
            runCellBalancing();
        }  

        printBatteryCellVoltages(modules);
    }
}

FSM_STATE normalOpTransition() {
    if (bms_fault || comm_fault || n_fault || OVUV_fault || OTUT_fault) {
        digitalWrite(FAULT_PIN, LOW);
        Serial.println("Fault Detected");
        // delay(5000);
        // HERE: need to switch to appropriate FAULT STATE by returning.
        if (comm_fault) {
            return FAULT_COMM;
        }
        else if (OVUV_fault || OTUT_fault) {
            return FAULT_TMPVOLT;
        }
        else if (bms_fault || n_fault) {
            return FAULT_UNEXPECTED;
        }
    }
    else {
        digitalWrite(FAULT_PIN, HIGH);
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
    Serial.println(comm_fault);
    Serial.println("Comm fault action");
    Serial.println("Communications Fault");
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