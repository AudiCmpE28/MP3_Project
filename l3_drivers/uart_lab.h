#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

typedef enum {
  UART_2,
  UART_3,
} uart_number_e;

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate);

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte);

// Writing using the THR register
bool uart_lab__polled_put(uart_number_e uart, char output_byte);

////////////part 2

void uart__enable_receive_interrupt(uart_number_e uart_number, uint16_t queue_Size);
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout);