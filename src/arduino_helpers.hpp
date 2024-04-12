

#ifndef ARD_HELPERS_HPP
#define ARD_HELPERS_HPP

#include <Arduino.h>
#include <stdint.h>
#include "bq79616.hpp"

#define FAULT_PIN D3
#define NFAULT_PIN D2
#define IMD_STATUS D4
#define POS_AIR_STATUS D9
#define NEG_AIR_STATUS D8


#define GET_TEMP(resistance)                          \
    (1 / (a1 + b1 * (log(resistance / 10000.0)) +     \
          c1 * (pow(log(resistance / 10000.0), 2)) +  \
          d1 * (pow(log(resistance / 10000.0), 3))) - \
     273.15)
#define GET_RESISTANCE(voltage) ((10000 * voltage) / (5 - voltage))


const float_t a1 = 0.003354016;
const float_t b1 = 0.000256524;
const float_t c1 = 2.61E-6;
const float_t d1 = 6.33E-8;

uint8_t read_signal_pins(void);
void read_cell_voltages(BMS_status * modules);
void read_cell_temps(BMS_status * modules);
void read_die_temps(BMS_status * modules);
void read_faults(BMS_status * modules);
void printBatteryCellVoltages(BMS_status * modules);
float_t sum_voltages(BMS_status * modules);
uint8_t calc_soc(uint16_t pack_voltage);
uint16_t calc_min_max_temp(BMS_status * modules);
uint32_t calc_min_max_volts(BMS_status * modules);

#endif