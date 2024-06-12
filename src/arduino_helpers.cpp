#include "arduino_helpers.hpp"

uint8_t read_signal_pins(void){
    uint8_t status = 0;
    status |= !digitalRead(NFAULT_PIN);
    status |= digitalRead(IMD_STATUS) << 1;
    status |= digitalRead(POS_AIR_STATUS) << 2;
    status |= digitalRead(NEG_AIR_STATUS) << 3;
    status |= digitalRead(RESET_PIN) << 4;
    return status;
}

void read_cell_voltages(BMS_status * modules){

    char voltage_response_frame[(16 * 2 + 6) * STACK_DEVICES];  // hold all 16 vcell*_hi/lo values
    memset(voltage_response_frame, 0, sizeof(voltage_response_frame));
    uint16_t raw_data = 0;

    // VOLTAGE SENSE (EVERY 9ms, so every 5 loops of 2ms each)
    ReadReg(0, VCELL16_HI + ((16 - ACTIVECHANNELS) * 2), voltage_response_frame, ACTIVECHANNELS * 2, 0, FRMWRT_STK_R); // 494 us

    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
    // printConsole("B%d voltages:\t", TOTALBOARDS - cb - 1);
    for (int i = 0; i < ACTIVECHANNELS; i++)
    {

        int boardcharStart = (ACTIVECHANNELS * 2 + 6) * cb;
        raw_data = (((voltage_response_frame[boardcharStart + (i * 2) + 4] & 0xFF) << 8) | (voltage_response_frame[boardcharStart + (i * 2) + 5] & 0xFF));
        uint16_t temp_voltage = (uint16_t)(Complement(raw_data, 0.19073)) / 10;
        // modules[cb].cell_voltages[ACTIVECHANNELS-i] = (uint8_t)(((uint16_t)(Complement(raw_data, 0.19073))) / 10.0);
        if(temp_voltage <= 250)
        {
        modules[cb].cell_voltages[ACTIVECHANNELS-i] = 0;
        // printConsole("Cell %d, %.3f ",i, (modules[cb].cell_voltages[i]) / 100.0 );

        }
        else{
        modules[cb].cell_voltages[ACTIVECHANNELS - i] = (uint8_t)(((uint16_t)(Complement(raw_data, 0.19073))) / 10.0 - 250);
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
        modules[cb].cell_temps[CELL_TEMP_NUM - i] = 255;
        }
        else{
        modules[cb].cell_temps[CELL_TEMP_NUM - i] = (uint8_t)((uint16_t)(GET_TEMP(GET_RESISTANCE((temp_voltage / 1000.0)))));
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
        raw_data = ((die_temp_response_frame[4] & 0xFF) << 8) | (die_temp_response_frame[5] & 0xFF); //The data is stored in index 4 and 5 because the first is the packet init byte, then dev addr byte, then 2 for the register address, then the data we want, then at the end the CRC
        temp_voltage = (uint16_t)(raw_data * 0.025);
        modules[cb].die_temp = (uint8_t) temp_voltage;
        // printConsole("%i ", modules[cb].cell_temps[i]);
    
    // printConsole("\n\r"); // newline per board
    }

}

void read_faults(BMS_status * modules){
    char fault_response_frame[(2 + 6) * STACK_DEVICES];  
    memset(fault_response_frame, 0, sizeof(fault_response_frame));
    ReadReg(0, FAULT_SUMMARY, fault_response_frame, 1, 0, FRMWRT_STK_R);
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
        int boardcharStart = (1 + 6) * cb;
        modules[cb].faults = fault_response_frame[boardcharStart + 4];
    }  
}

float sum_voltages(BMS_status * modules){
    float pack_voltage = 0.0;

    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
        for (int i = 0; i < 16; i++)
        {
            pack_voltage += (modules[cb].cell_voltages[i] + 250.0) / 100;
        }
    }  
    return pack_voltage;
}

uint8_t calc_soc(uint16_t pack_voltage){
    // Ensure voltage is within the expected range
    if (pack_voltage > FULLY_CHARGED_VOLTAGE){
        pack_voltage = FULLY_CHARGED_VOLTAGE;
    }
    if (pack_voltage < FULLY_DISCHARGED_VOLTAGE) {
        pack_voltage = FULLY_DISCHARGED_VOLTAGE;
    }
    
    // Linear interpolation to calculate SoC
    uint8_t soc = ((pack_voltage - FULLY_DISCHARGED_VOLTAGE) / 
                (FULLY_CHARGED_VOLTAGE - FULLY_DISCHARGED_VOLTAGE)) * 100;
    // uint8_t soc = 100;
    return soc;
}

uint16_t calc_min_max_temp(BMS_status * modules){
    int8_t min_temp = 120;
    int8_t max_temp = -120;
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
        for (int i = 0; i < 8; i++)
        {
            if(min_temp > modules[cb].cell_temps[i])
                min_temp = modules[cb].cell_temps[i];
            else if(max_temp < modules[cb].cell_temps[i])
                max_temp = modules[cb].cell_temps[i];
        }
    }  
    return static_cast<uint16_t>(max_temp << 8 | min_temp);
}

uint32_t calc_min_max_volts(BMS_status * modules){
    uint32_t min_volts = 65000;
    uint32_t max_volts = 0;
    for (uint16_t cb = 0; cb < STACK_DEVICES; cb++)
    {
        for (int i = 0; i < 16; i++)
        {
            float_t cur_cell = ((modules[cb].cell_voltages[i] + 250.0) / 100) * 10000;
            if(min_volts > cur_cell)
                min_volts = cur_cell;
            else if(max_volts < cur_cell)
                max_volts = cur_cell;
            
        }
    }  
    return static_cast<uint32_t>(((min_volts & 0xFFFF) << 16) | (max_volts & 0xFFFF));
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

void printBatteryCellTemps(BMS_status * modules) {

    for (int i = 0; i < 6; i++) {
        Serial.print("Pack ");
        Serial.print(i+1);
        for (size_t j = 0; j < 8; j++) {
            // Untransform the value
            uint8_t temp = modules[i].cell_temps[j];
            
            // Print the untransformed voltage
            Serial.print("\t");
            Serial.print(temp); // Print with 2 decimal places
        }
        Serial.println(); // Move to the next line for the next pack
    }
}

