#include "can_helpers.hpp"

#include "stdint.h"
#include "math.h"


//Methods to convert the integers into bytes to be sent via canbus
char* int_to_bytes(uint32_t input){
    char bytes[4] = {((input >> 24) & 0xFF), ((input >> 16) & 0xFF), ((input >> 8) & 0xFF), (input & 0xFF)};

    return bytes;
}

char* int_to_bytes(uint16_t input){
    char bytes[2] = {((input >> 16) & 0xFF), (input & 0xFF)};

    return bytes;
}

char* float_to_4bytes(float input){
    uint32_t integer_from_float = round(input * 100);

    return int_to_bytes(integer_from_float);
}

char* float_to_2bytes(float input){
    uint16_t integer_from_float = round(input * 100);

    return int_to_bytes(integer_from_float);
}
