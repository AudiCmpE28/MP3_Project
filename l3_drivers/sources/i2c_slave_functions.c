#include "i2c_slave_functions.h"

// static volatile uint8_t slave_memory[256];
static uint8_t slave_memory[256];
const uint32_t clk_bus_speed_in_hz = UINT32_C(400) * 1000;
/***********************************************************************************************************
 *                          Lab 9: I2C Master-Slave Using Same board
 ***********************************************************************************************************/
// MASTER
void i2c_ONE__init_slave(uint8_t get_slave_address_to_respond) {
  // Table 84: P0.0 for SDA
  LPC_IOCON->P0_0 |= (1 << 10);
  LPC_IOCON->P0_0 &= ~0b111;
  LPC_IOCON->P0_0 |= 0b011; // I2C1_SDA

  // Table 84: P0.1 for SCL
  LPC_IOCON->P0_1 |= (1 << 10);
  LPC_IOCON->P0_1 &= ~0b111;
  LPC_IOCON->P0_1 |= 0b011; // I2C1_SCL

  i2c__initialize(I2C__1, clk_bus_speed_in_hz, clock__get_peripheral_clock_hz());

  // Set Slave address register as an input byte; Shift left by 1 to format ADR register
  LPC_I2C1->ADR1 = get_slave_address_to_respond;
  LPC_I2C1->CONSET = 0x44;

  for (unsigned addr = 2; addr < 255; addr += 2) {
    if (i2c__detect(I2C__2, addr)) {
      printf("I2C 1 slave detected at address: 0x%02X\n", addr);
    }
  }
}

// SLAVE
void i2c_TWO__init_slave(uint8_t get_slave_address_to_respond) {
  LPC_I2C2->ADR2 = get_slave_address_to_respond;
  LPC_I2C2->CONSET = 0x44;
}

// as long as memory index is within range, we can read the value of that location
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index <= 255) {
    *memory = slave_memory[memory_index];
    return true;
  }
  return false;
}

// as long as memory index within range, we can write in that specific location
bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index <= 255) {
    slave_memory[memory_index] = memory_value;
    return true;
  }
  return false;
}