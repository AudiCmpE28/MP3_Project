#pragma once

#include "gpio.h"
#include <stdint.h>
#include <stdlib.h>

/**
 * This configures what DMA channels the SSP2 driver utilizes
 * for ssp2__dma_write_block() and ssp2__dma_read_block()
 */
#define SSP2__DMA_TX_CHANNEL 0
#define SSP2__DMA_RX_CHANNEL 1

/// Initialize the bus with the given maximum clock rate in Khz
void ssp2__initialize(uint32_t max_clock_khz);

/// After initialization, this allows you to change the bus clock speed
void ssp2__set_max_clock(uint32_t max_clock_khz);

/**
 * Exchange a single byte over the SPI bus
 * @returns the byte received while sending the byte_to_transmit
 */
uint8_t ssp2__exchange_byte(uint8_t byte_to_transmit);

/**
 * @{
 * @name    Exchanges larger blocks over the SPI bus
 * These are designed to be one-way transmission for the SPI for now
 */
void ssp2__dma_write_block(const unsigned char *output_block, size_t number_of_bytes);
void ssp2__dma_read_block(unsigned char *input_block, size_t number_of_bytes);
/** @} */

//////////////// Lab 6
void ssp2__init(uint32_t max_clock_mhz);
uint8_t ssp2__exchange_byte_personal(uint8_t data_out); // sends/recieves

// configure port and pins
void ssp2_configure_flash();                      // configures SCK, MISO, MOSI
void adesto_flash_send_address(uint32_t address); // sends address 24 bits
void adesto_chip_select(int state);               // selects device when 1, else deselects

///////////////////////////PART 3 EC
void write_enable(void);  // enables write enable
void write_disable(void); // disables write enable
void adesto_read_at_page_address(uint32_t address, uint8_t *output);
void page_erase_set_high(uint32_t address);
void adesto_write_at_page_address(uint32_t address, int direction_input);

// #pragma once

// #include "gpio.h"
// #include <stdint.h>
// #include <stdlib.h>

// /**
//  * This configures what DMA channels the SSP2 driver utilizes
//  * for ssp2__dma_write_block() and ssp2__dma_read_block()
//  */
// #define SSP2__DMA_TX_CHANNEL 0
// #define SSP2__DMA_RX_CHANNEL 1

// /// Initialize the bus with the given maximum clock rate in Khz
// void ssp2__initialize(uint32_t max_clock_khz);

// /// After initialization, this allows you to change the bus clock speed
// void ssp2__set_max_clock(uint32_t max_clock_khz);

// /**
//  * Exchange a single byte over the SPI bus
//  * @returns the byte received while sending the byte_to_transmit
//  */
// uint8_t ssp2__exchange_byte(uint8_t byte_to_transmit);

// /**
//  * @{
//  * @name    Exchanges larger blocks over the SPI bus
//  * These are designed to be one-way transmission for the SPI for now
//  */
// void ssp2__dma_write_block(const unsigned char *output_block, size_t number_of_bytes);
// void ssp2__dma_read_block(unsigned char *input_block, size_t number_of_bytes);
// /** @} */

// //////////////// Lab 6
// void ssp2__init(uint32_t max_clock_mhz);
// uint8_t ssp2__exchange_byte_personal(uint8_t data_out); // sends/recieves

// // configure port and pins
// void ssp2_configure_flash();                      // configures SCK, MISO, MOSI
// void adesto_flash_send_address(uint32_t address); // sends address 24 bits
// void adesto_chip_select(int state);               // selects device when 1, else deselects

// ///////////////////////////PART 3 EC
// void write_enable(void);  // enables write enable
// void write_disable(void); // disables write enable
// void adesto_read_at_page_address(uint32_t address, uint8_t *output);
// void page_erase_set_high(uint32_t address);
// void adesto_write_at_page_address(uint32_t address, int direction_input);