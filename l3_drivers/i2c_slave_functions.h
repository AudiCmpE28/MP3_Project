#pragma once
#include "clock.h"
#include "gpio.h"
#include "i2c.h"
#include "lpc40xx.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***********************************************************************************************************
 *                          Lab 10: New Functions
 ***********************************************************************************************************/
void i2c_ONE__init_slave(uint8_t get_slave_address_to_respond);

void i2c_TWO__init_slave(uint8_t get_slave_address_to_respond); // to use same board
/*
    Use memory_index & read the data to *memory pointer
    return true if everything is well

*/
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory);

/*
    Use memory_index & write memory_value
    return true if this write operation was valid

*/
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value);
// TODO: write the implementation of these fucntions in main.c