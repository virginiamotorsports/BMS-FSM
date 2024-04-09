#ifndef CANHELPERS_HPP
#define CANHELPERS_HPP

#include "stdint.h"
#include "math.h"



enum CAN_Msgs
{
    START_ITEM = 0x100,
    STATUS = START_ITEM,
    FAULT,
    CELL_TEMP_0,
    CELL_TEMP_1,
    CELL_TEMP_2,
    CELL_TEMP_3,
    CELL_TEMP_4,
    CELL_TEMP_5,
    CELL_TEMP_6,
    CELL_TEMP_7,
    CELL_TEMP_8,
    CELL_TEMP_9,
    CELL_TEMP_10,
    CELL_TEMP_11,
    CELL_VOLTAGE_0,
    CELL_VOLTAGE_1,
    CELL_VOLTAGE_2,
    CELL_VOLTAGE_3,
    CELL_VOLTAGE_4,
    CELL_VOLTAGE_5,
    CELL_VOLTAGE_6,
    CELL_VOLTAGE_7,
    CELL_VOLTAGE_8,
    CELL_VOLTAGE_9,
    CELL_VOLTAGE_10,
    CELL_VOLTAGE_11,
    CELL_VOLTAGE_12,
    CELL_VOLTAGE_13,
    CELL_VOLTAGE_14,
    CELL_VOLTAGE_15,
    CELL_VOLTAGE_16,
    CELL_VOLTAGE_17,
    CELL_VOLTAGE_18,
    CELL_VOLTAGE_19,
    CELL_VOLTAGE_20,
    CELL_VOLTAGE_21,
    CELL_VOLTAGE_22,
    CELL_VOLTAGE_23,
    LAST_ITEM
};

#define ORION_MSG_1 0x6B0 
/*
Signal Start_bit Length_[bit] Value_Type Factor Offset Min Max
Pack_Current 8 16 Signed 0.1 0 -32768 3276.7
Pack_Inst_Voltage 24 16 Unsigned 0.1 0 0 6553.5
Pack_SOC 32 8 Unsigned 0.5 0 0 127.5
MPI2_State 40 1 Unsigned 1 0 0 1
MPI3_State 41 1 Unsigned 1 0 0 1
MPO2_State 43 1 Unsigned 1 0 0 1
MPO3_State 44 1 Unsigned 1 0 0 1
MPO4_State 45 1 Unsigned 1 0 0 1
MP_Enable_State 46 1 Unsigned 1 0 0 1
MPO1_State 47 1 Unsigned 1 0 0 1
Discharge_Relay_State 48 1 Unsigned 1 0 0 1
Charge_Relay_State 49 1 Unsigned 1 0 0 1
Charger_Safety_State 50 1 Unsigned 1 0 0 1
MIL_State 51 1 Unsigned 1 0 0 1
MPI1_State 52 1 Unsigned 1 0 0 1
AlwaysOn_State 53 1 Unsigned 1 0 0 1
Is_Ready_State 54 1 Unsigned 1 0 0 1
Is_Charging_State 55 1 Unsigned 1 0 0 1
*/

#define ORION_MSG_2 0x6B0
/*
Signal Start_bit Length_[bit] Value_Type Factor Offset Min Max
Pack_DCL 8 16 Unsigned 1 0 0 65535
Pack_CCL 16 8 Unsigned 1 0 0 255
Pack_High_Temp 32 8 Signed 1 0 -128 127
Pack_Low_Temp 40 8 Signed 1 0 -128 127
*/


#define ORION_MSG_3 0x6B0
/*
Signal Start_bit Length_[bit] Value_Type Factor Offset Min Max
Low_Cell_Voltage 8 16 Unsigned 0.0001 0 0 6.5535
High_Cell_Voltage 24 16 Unsigned 0.0001 0 0 6.5535
*/


const uint8_t message_data_width = LAST_ITEM - STATUS;


//Methods to convert the integers into bytes to be sent via canbus
char* int_to_bytes(uint32_t input);

char* int_to_bytes(uint16_t input);

char* float_to_4bytes(float input);

char* float_to_2bytes(float input);


#endif