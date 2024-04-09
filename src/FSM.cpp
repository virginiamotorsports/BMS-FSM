#include "FSM.h"
#include "can_helpers.hpp"


#define FAULT_PIN D3
#define NFAULT_PIN D2

#define GET_TEMP(resistance)                          \
    (1 / (a1 + b1 * (log(resistance / 10000.0)) +     \
          c1 * (pow(log(resistance / 10000.0), 2)) +  \
          d1 * (pow(log(resistance / 10000.0), 3))) - \
     273.15)
#define GET_RESISTANCE(voltage) ((10000 * voltage) / (5 - voltage))

extern uint8_t loop_counter;

const float_t a1 = 0.003354016;
const float_t b1 = 0.000256524;
const float_t c1 = 2.61E-6;
const float_t d1 = 6.33E-8;

unsigned long lastMillis = millis();

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
char voltage_response_frame[(16 * 2 + 6) *
TOTALBOARDS];  // hold all 16 vcell*_hi/lo values
char temp_response_frame[(8 * 2 + 6) *
TOTALBOARDS];  // hold all 16 vcell*_hi/lo values
char response_frame_current[(1 + 6)];   //

bool communicationsOnOff = false;
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

  // CanMsg * msg;
//   uint8_t msg_data[] = {0,0,0,0,0,0,0,0};

  for(uint32_t addr = START_ITEM; addr < LAST_ITEM; addr++){
    // memcpy((void *)(msg_data), &message_data[addr], sizeof(message_data[addr]));
    // Serial.println(addr);
    // *msg = CanMsg(0x400, sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]);

    int ret = CAN.write(CanMsg(CanStandardId(addr), sizeof(message_data[addr - START_ITEM]), message_data[addr - START_ITEM]));
    if(ret != 0){
      Serial.println("CAN Error");
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
    memset(temp_response_frame, 0, sizeof(temp_response_frame));
    memset(voltage_response_frame, 0, sizeof(voltage_response_frame));
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
        n_fault = !digitalRead(NFAULT_PIN);

        WriteReg(0, OVUV_CTRL, 0x05, 1, FRMWRT_STK_W);  // run OV UV
        WriteReg(0, OTUT_CTRL, 0x05, 1, FRMWRT_STK_W);  // run OT UT
        ReadReg(0, FAULT_SUMMARY, response_frame_current, 1, 0, FRMWRT_STK_R);  // 175 us

        // printResponseFrameForDebug();

        OVUV_fault = ((response_frame_current[4] & 0x04) ? true : false);
        OTUT_fault = ((response_frame_current[4] & 0x08) ? true : false);

        // VOLTAGE SENSE
        ReadReg(0, VCELL16_HI + (16 - ACTIVECHANNELS) * 2, voltage_response_frame, ACTIVECHANNELS * 2, 0, FRMWRT_STK_R);  // 494 us

        for (uint16_t cb = 0;
                cb < (BRIDGEDEVICE == 1 ? TOTALBOARDS - 1 : TOTALBOARDS);
                cb++) {
            printConsole("B%d voltages: ", TOTALBOARDS - cb - 1);
            for (int i = 0; i < ACTIVECHANNELS; i++) {
                int boardcharStart = (ACTIVECHANNELS * 2 + 6) * cb;
                raw_data =
                    (((voltage_response_frame[boardcharStart + (i * 2) +
                        4] &
                        0xFF)
                        << 8) |
                        (voltage_response_frame[boardcharStart + (i * 2) + 5] &
                        0xFF));
                modules[cb].cell_voltages[i] =
                    (uint16_t)(Complement(raw_data, 0.19073));
                if (i != 15) {
                    printConsole("%.3f, ",
                                    modules[cb].cell_voltages[i] / 1000.0);
                }
                else {
                    printConsole("%.3f",
                                    modules[cb].cell_voltages[i] / 1000.0);
                }
            }
            printConsole("\n\r");  // newline per board
        }

        // Cell Temp Sense
        ReadReg(0, GPIO1_HI, temp_response_frame, CELL_TEMP_NUM * 2, 0,
                FRMWRT_STK_R);  // 494 us
        uint16_t die_temp_voltage = 0;
        for (uint16_t cb = 0;
                cb < (BRIDGEDEVICE == 1 ? TOTALBOARDS - 1 : TOTALBOARDS);
                cb++) {
            printConsole("B%d temps: ", TOTALBOARDS - cb - 1);
            for (int i = 0; i < CELL_TEMP_NUM; i++) {
                int boardcharStart = (CELL_TEMP_NUM * 2 + 6) * cb;
                raw_data =
                    ((temp_response_frame[boardcharStart + (i * 2) + 4] &
                        0xFF)
                        << 8) |
                    (temp_response_frame[boardcharStart + (i * 2) + 5] &
                        0xFF);
                die_temp_voltage = (uint16_t)(raw_data * 0.15259);
                if (die_temp_voltage >= 4800) {
                    modules[cb].die_temp = 1000;
                }
                else {
                    modules[cb].die_temp = (uint16_t)(GET_TEMP(GET_RESISTANCE((die_temp_voltage / 1000.0))) * 10);
                }
                printConsole("%.1f, ", modules[cb].cell_temps[i] / 10.0);
            }
            printConsole("\n\r");  // newline per board
        }

        // Die Temp Sense
        ReadReg(0, DIETEMP1_HI, temp_response_frame, 2, 0, FRMWRT_STK_R); // 494 us
        uint16_t temp_voltage = 0;
        for (uint16_t cb = 0; cb < (BRIDGEDEVICE == 1 ? TOTALBOARDS - 1 : TOTALBOARDS); cb++)
        {
        printConsole("B%d temps:\t", TOTALBOARDS - cb - 1);
        for (int i = 0; i < CELL_TEMP_NUM; i++)
        {
            int boardcharStart = (CELL_TEMP_NUM * 2 + 6) * cb;
            raw_data = ((temp_response_frame[boardcharStart + (i * 2) + 4] & 0xFF) << 8) | (temp_response_frame[boardcharStart + (i * 2) + 5] & 0xFF);
            temp_voltage = (uint16_t)(raw_data * 0.15259);
            if(temp_voltage >= 4800)
            {
            modules[cb].cell_temps[i] = 255;
            }
            else{
            modules[cb].cell_temps[i] = (uint8_t)((uint16_t)(GET_TEMP(GET_RESISTANCE((temp_voltage / 1000.0)))));
            }
            printConsole("%i ", modules[cb].cell_temps[i]);
        }
        printConsole("\n\r"); // newline per board
        }

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