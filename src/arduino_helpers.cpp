#include "arduino_helpers.hpp"

uint8_t read_signal_pins(void){
    uint8_t status = 0;
    status |= !digitalRead(NFAULT_PIN);
    status |= !digitalRead(IMD_STATUS) << 1;
    status |= digitalRead(POS_AIR_STATUS) << 2;
    status |= digitalRead(NEG_AIR_STATUS) << 3;
    return status;
}

void read_cell_voltages(BMS_status * modules){

    char voltage_response_frame[(16 * 2 + 6) * STACK_DEVICES];  // hold all 16 vcell*_hi/lo values
    memset(voltage_response_frame, 0, sizeof(voltage_response_frame));
    uint16_t raw_data = 0;

    // VOLTAGE SENSE (EVERY 9ms, so every 5 loops of 2ms each)
    ReadReg(0, VCELL16_HI + (16 - ACTIVECHANNELS) * 2, voltage_response_frame, ACTIVECHANNELS * 2, 0, FRMWRT_STK_R); // 494 us

    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
    // printConsole("B%d voltages:\t", TOTALBOARDS - cb - 1);
    for (int i = 0; i < ACTIVECHANNELS; i++)
    {
        int boardcharStart = (ACTIVECHANNELS * 2 + 6) * cb;
        raw_data = (((voltage_response_frame[boardcharStart + (i * 2) + 4] & 0xFF) << 8) | (voltage_response_frame[boardcharStart + (i * 2) + 5] & 0xFF));
        uint16_t temp_voltage = (uint16_t)(Complement(raw_data, 0.19073)) / 10;
        modules[cb].cell_voltages[i] = (uint8_t)(((uint16_t)(Complement(raw_data, 0.19073))) / 10.0);
        if(temp_voltage <= 250)
        {
        modules[cb].cell_voltages[i] = 0;
        // printConsole("Cell %d, %.3f ",i, (modules[cb].cell_voltages[i]) / 100.0 );

        }
        else{
        modules[cb].cell_voltages[i] = (uint8_t)(((uint16_t)(Complement(raw_data, 0.19073))) / 10.0 - 250);
        // printConsole("Cell %d, %.3f ",i, (modules[cb].cell_voltages[i] + 250) / 100.0 );

        }
    }
    // printConsole("\n\r"); // newline per board
    }

}
void read_cell_temps(BMS_status * modules){
    char cell_temp_response_frame[(8 * 2 + 6) * STACK_DEVICES];  // hold all 16 vcell*_hi/lo values
    uint16_t raw_data = 0;
    memset(cell_temp_response_frame, 0, sizeof(cell_temp_response_frame));

    // VOLTAGE SENSE (EVERY 9ms, so every 5 loops of 2ms each)
    ReadReg(0, GPIO1_HI, cell_temp_response_frame, CELL_TEMP_NUM * 2, 0, FRMWRT_STK_R); // 494 us
    uint16_t temp_voltage = 0;
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
    // printConsole("B%d temps:\t", STACK_DEVICES - cb - 1);
    for (int i = 0; i < CELL_TEMP_NUM; i++)
    {
        int boardcharStart = (CELL_TEMP_NUM * 2 + 6) * cb;
        raw_data = ((cell_temp_response_frame[boardcharStart + (i * 2) + 4] & 0xFF) << 8) | (cell_temp_response_frame[boardcharStart + (i * 2) + 5] & 0xFF);
        temp_voltage = (uint16_t)(raw_data * 0.15259);
        if(temp_voltage >= 4800)
        {
        modules[cb].cell_temps[i] = 255;
        }
        else{
        modules[cb].cell_temps[i] = (uint8_t)((uint16_t)(GET_TEMP(GET_RESISTANCE((temp_voltage / 1000.0)))));
        }
        // printConsole("%i ", modules[cb].cell_temps[i]);
    }
    // printConsole("\n\r"); // newline per board
    }


}
void read_die_temps(BMS_status * modules){
    char die_temp_response_frame[(2 + 6) * STACK_DEVICES]; 
    uint16_t raw_data = 0;
    memset(die_temp_response_frame, 0, sizeof(die_temp_response_frame));


    ReadReg(0, DIETEMP1_HI, die_temp_response_frame, 2, 0, FRMWRT_STK_R); // 494 us
    uint16_t temp_voltage = 0;
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
    // printConsole("B%d temps:\t", TOTALBOARDS - cb - 1);
    for (int i = 0; i < CELL_TEMP_NUM; i++)
    {
        int boardcharStart = (CELL_TEMP_NUM * 2 + 6) * cb;
        raw_data = ((die_temp_response_frame[boardcharStart + (i * 2) + 4] & 0xFF) << 8) | (die_temp_response_frame[boardcharStart + (i * 2) + 5] & 0xFF);
        temp_voltage = (uint16_t)(raw_data * 0.15259);
        if(temp_voltage >= 4800)
        {
        modules[cb].cell_temps[i] = 255;
        }
        else{
        modules[cb].cell_temps[i] = (uint8_t)((uint16_t)(GET_TEMP(GET_RESISTANCE((temp_voltage / 1000.0)))));
        }
        // printConsole("%i ", modules[cb].cell_temps[i]);
    }
    // printConsole("\n\r"); // newline per board
    }

}

void read_faults(BMS_status * modules){
    char fault_response_frame[(2 + 6) * STACK_DEVICES];  
    memset(fault_response_frame, 0, sizeof(fault_response_frame));

    ReadReg(0, FAULT_SUMMARY, fault_response_frame, 1, 0, FRMWRT_STK_R);
    
}

uint16_t sum_voltages(BMS_status * modules){
    uint16_t pack_voltage = 0;
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
    for (int i = 0; i < 16; i++)
    {
        pack_voltage += modules[cb].cell_voltages[i];
    }
    }  
    return pack_voltage;
}


void printBatteryCellVoltages(BMS_status * modules) {

    for (int i = 0; i < 6; i++) {
        Serial.print("Pack ");
        Serial.print(i+1);
        for (size_t j = 0; j < 16; j++) {
            // Untransform the value
            float voltage = (modules[i].cell_voltages[j] + 250.0) / 100.0;
            
            // Print the untransformed voltage
            Serial.print("\t");
            Serial.print(voltage, 2); // Print with 2 decimal places
        }
        Serial.println(); // Move to the next line for the next pack
    }
}

