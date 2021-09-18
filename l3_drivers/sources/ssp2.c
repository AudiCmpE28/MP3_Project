#include <stdbool.h>
#include <stddef.h>

#include "ssp2.h"

#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

/**
 * These are the DMA request numbers that the hardware maps
 * See LPC40xx user manual, chapter: General Purpose DMA controller, Table 696: DMA connections
 */
#define SSP2__DMA_REQUEST_TX 6UL
#define SSP2__DMA_REQUEST_RX 7UL

/**
 * There are only 8 DMA channels on the LPC40xx so confirm validity
 */
#if !(SSP2__DMA_TX_CHANNEL >= 0 && SSP2__DMA_TX_CHANNEL <= 7)
#error "SSP2__DMA_TX_CHANNEL must be between 0 and 7"
#endif
#if !(SSP2__DMA_RX_CHANNEL >= 0 && SSP2__DMA_RX_CHANNEL <= 7)
#error "SSP2__DMA_RX_CHANNEL must be between 0 and 7"
#endif

typedef enum {
  SSP_DMA__ERROR_NONE = 0,
  SSP_DMA__ERROR_LENGTH = 1,
  SSP_DMA__ERROR_BUSY = 2,
} ssp_dma_error_e;

/**
 * Initialize the DMA struct pointers that we will use
 */
static const size_t ssp2__dma_channel_memory_spacing = (LPC_GPDMACH1_BASE - LPC_GPDMACH0_BASE);
static LPC_GPDMACH_TypeDef *ssp2__dma_tx =
    (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + (SSP2__DMA_TX_CHANNEL * ssp2__dma_channel_memory_spacing));
static LPC_GPDMACH_TypeDef *ssp2__dma_rx =
    (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + (SSP2__DMA_RX_CHANNEL * ssp2__dma_channel_memory_spacing));

static void ssp2__dma_init(void);
static ssp_dma_error_e ssp2__dma_transfer_block(unsigned char *buffer_pointer, uint32_t num_bytes, bool is_write_op);

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void ssp2__initialize(uint32_t max_clock_khz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);

  LPC_SSP2->CR0 = 7;        // 8-bit mode
  LPC_SSP2->CR1 = (1 << 1); // Enable SSP as Master
  ssp2__set_max_clock(max_clock_khz);

  ssp2__dma_init();
}

void ssp2__set_max_clock(uint32_t max_clock_khz) {
  uint8_t divider = 2;
  const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;

  // Keep scaling down divider until calculated is higher
  while (max_clock_khz < (cpu_clock_khz / divider) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP2->CPSR = divider;
}

uint8_t ssp2__exchange_byte(uint8_t byte_to_transmit) {
  LPC_SSP2->DR = byte_to_transmit;

  while (LPC_SSP2->SR & (1 << 4)) {
    ; // Wait until SSP is busy
  }

  return (uint8_t)(LPC_SSP2->DR & 0xFF);
}

void ssp2__dma_write_block(const unsigned char *output_block, size_t number_of_bytes) {
  const bool is_write_operation = true;
  ssp2__dma_transfer_block((unsigned char *)output_block, number_of_bytes, is_write_operation);
}

void ssp2__dma_read_block(unsigned char *input_block, size_t number_of_bytes) {
  const bool is_write_operation = true;
  ssp2__dma_transfer_block(input_block, number_of_bytes, !is_write_operation);
}

void ssp2__dma_init(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__GPDMA);

  const uint32_t enable_bitmask = (1 << 0);
  LPC_GPDMA->Config = enable_bitmask;
  while (!(LPC_GPDMA->Config & enable_bitmask)) {
    ; // Wait for DMA to be enabled
  }

  // SSP dma channels will not use linked list transfers
  ssp2__dma_rx->CLLI = 0;
  ssp2__dma_tx->CLLI = 0;

  // These registers are only initialized once, but this optimization makes no measurable difference
  // ssp2__dma_tx->CDestAddr = (uint32_t)(&(LPC_SSP2->DR));
  // ssp2__dma_rx->CSrcAddr = (uint32_t)(&(LPC_SSP2->DR));
}

ssp_dma_error_e ssp2__dma_transfer_block(unsigned char *buffer_pointer, uint32_t num_bytes, bool is_write_op) {
  uint32_t dummyBuffer = 0xffffffff;

  // DMA is limited to 12-bit transfer size
  if (num_bytes >= 0x1000) {
    return SSP_DMA__ERROR_LENGTH;
  }

  // DMA channels should not be busy
  if ((ssp2__dma_tx->CConfig & 1) || (ssp2__dma_rx->CConfig & 1)) {
    return SSP_DMA__ERROR_BUSY;
  }

  /**
   * TO DO : Optimize SSP1 DMA
   *  - Try setting source and destination burst size to 4
   *  - Try setting 16-bit SPI, and source and destination width to 1 WORD
   *
   * LPC_SSP2->CR0 : B3:B0. 0b0111 = 8-bit and 0b1111 = 16-bit
   * LPC_SSP2->CR0 |=  (1<<3);  // 16-bit
   * LPC_SSP2->CR0 &= ~(1<<3);  // 8-bit
   *
   * Bits of DMACCControl:
   * Transfer size: B11:B0
   * Source Burst Size: B14:B12   - 0:1, 1:4, 2:8, 3:16 bytes
   * Dest.  Burst Size: B17:B15   - 0:1, 1:4, 2:8, 3:16 bytes
   * Source Width: B20:B18        - 0:Byte, 1:Word, 2:DWORD
   * Dest.  Width: B23:B21        - 0:Byte, 1:Word, 2:DWORD
   * Source increment: B26
   * Destination increment: B27
   * Terminal count interrupt enable: B31
   *
   * Bits of DMACCConfig:
   * Enable: B0
   * Source Peripheral: B5:B1   (ignored if source is memory)
   * Dest.  Peripheral: B10:B6  (ignored if dest. is memory)
   * Transfer type: B13:B11 - See below
   * Interrupt Error Mask: B14
   * Terminal Count Interrupt : B15
   *
   * Bits for transfer type:
   * 000 - Memory to Memory
   * 001 - Memory to Peripheral
   * 010 - Peripheral to Memory
   * 011 - Source peripheral to destination peripheral
   */
  static const uint32_t increment_source_address = UINT32_C(1) << 26;
  static const uint32_t increment_destination_address = UINT32_C(1) << 27;

  static const uint32_t transfer_memory_to_peripheral = UINT32_C(1) << 11;
  static const uint32_t transfer_peripheral_to_memory = UINT32_C(2) << 11;
  static const uint32_t dma_enable = UINT32_C(1) << 0;

  static const uint32_t terminal_count_interrupt_enable = UINT32_C(1) << 31;

  /**
   * Clear existing terminal count and error interrupts otherwise
   * DMA will not start.
   */
  LPC_GPDMA->IntTCClear = (1 << SSP2__DMA_RX_CHANNEL) | (1 << SSP2__DMA_TX_CHANNEL);
  LPC_GPDMA->IntErrClr = (1 << SSP2__DMA_RX_CHANNEL) | (1 << SSP2__DMA_TX_CHANNEL);

  /**
   * From SPI to buffer:
   * For write operation :
   *      - Receive data into dummy buffer
   *      - Don't increment destination
   * For read operation:
   *      - Read data into buffer_pointer
   *      - Increment destination
   *
   * Note: Setting destination burst of 2 or 4 bytes makes no difference (1 << 15)
   */
  ssp2__dma_rx->CSrcAddr = (uint32_t)(&(LPC_SSP2->DR));
  if (is_write_op) {
    ssp2__dma_rx->CDestAddr = (uint32_t)(&dummyBuffer);
    ssp2__dma_rx->CControl = num_bytes | terminal_count_interrupt_enable;
  } else {
    ssp2__dma_rx->CDestAddr = (uint32_t)buffer_pointer;
    ssp2__dma_rx->CControl = num_bytes | increment_destination_address | terminal_count_interrupt_enable;
  }
  ssp2__dma_rx->CConfig = (SSP2__DMA_REQUEST_RX << 1) | transfer_peripheral_to_memory | dma_enable;

  /**
   * From buffer to SPI :
   * For write operation :
   *      - Source data is buffer_pointer
   *      - Increment source data
   * For read operation:
   *      - Source data is buffer with 0xFF
   *      - Don't increment source data
   */
  if (is_write_op) {
    ssp2__dma_tx->CSrcAddr = (uint32_t)(buffer_pointer);
    ssp2__dma_tx->CControl = num_bytes | increment_source_address;
  } else {
    ssp2__dma_tx->CSrcAddr = (uint32_t)(&dummyBuffer);
    ssp2__dma_tx->CControl = num_bytes;
  }
  ssp2__dma_tx->CDestAddr = (uint32_t)(&(LPC_SSP2->DR));
  ssp2__dma_tx->CConfig = (SSP2__DMA_REQUEST_TX << 6) | transfer_memory_to_peripheral | dma_enable;

  /**
   * Channel must be fully configured and then enabled separately.
   * Setting DMACR's Rx/Tx bits should trigger the DMA
   */
  static const uint32_t enable_rx_tx_dma_triggers = 0x03;
  LPC_SSP2->DMACR |= enable_rx_tx_dma_triggers;

  while ((ssp2__dma_rx->CControl & 0xfff)) {
    ; // Poll for DMA transfer to complete
  }
  LPC_SSP2->DMACR &= ~enable_rx_tx_dma_triggers;

  return SSP_DMA__ERROR_NONE;
}

/*   Lab 6   */
void ssp2__init(uint32_t max_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly (Table 482)
  /// a) Power on Peripheral
  LPC_SC->PCONP |= (1 << 20); // Table 16 bit 20, PCONP to power peripheral specifically

  /// b) Setup control registers CR0 and CR1
  LPC_SSP2->CR0 &= ~((1 << 5) | (1 << 4)); // Table 483 bit 5:4, 00 for SPI
  LPC_SSP2->CR1 |= (1 << 1);               // Table 484 bit 1, SSP controller "enabled"

  /// c) Setup prescalar register to be <= max_clock_mhz
  LPC_SSP2->CPSR = max_clock_mhz; // Clock Prescale Register
}

uint8_t ssp2__exchange_byte_personal(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  LPC_SSP2->DR = data_out;
  // Table 486, bit 4 (0 if idle, 1 if send/recieve)
  while (LPC_SSP2->SR & (1 << 4)) { // while bit is 1
    ;
  }
  return ((uint8_t)(LPC_SSP2->DR & 0xFF));
}

// configure port and pin numbers for ssp
void ssp2_configure_flash() {
  // table 84 funct 100 for sck, miso, mosi
  // gpio__construct_with_function(0, 0, GPIO__FUNCTION_4);
  // gpio__construct_with_function(0, 1, GPIO__FUNCTION_4);
  // gpio__construct_with_function(0, 4, GPIO__FUNCTION_4);
  LPC_IOCON->P1_0 &= ~0b111; // SCK2
  LPC_IOCON->P1_1 &= ~0b111; // MOSI2
  LPC_IOCON->P1_4 &= ~0b111; // MISO2

  LPC_IOCON->P1_0 |= 0b100;
  LPC_IOCON->P1_1 |= 0b100;
  LPC_IOCON->P1_4 |= 0b100;
}

/**
 * Adesto flash asks to send 24-bit address
 * We can use our usual uint32_t to store the address
 * and then transmit this address over the SPI driver
 * one byte at a time
 */
void adesto_flash_send_address(uint32_t address) {
  (void)ssp2__exchange_byte((address >> 16) & 0xFF);
  (void)ssp2__exchange_byte((address >> 8) & 0xFF);
  (void)ssp2__exchange_byte((address >> 0) & 0xFF);
}

void adesto_chip_select(int state) {
  if (state == 1) {
    gpio0__set_as_output(1, 10);
    gpio0__set_low(1, 10);
    gpio0__set_as_output(2, 0);
    gpio0__set_low(2, 0);
  } else if (state == 0) {
    gpio0__set_as_output(1, 10);
    gpio0__set_high(1, 10);
    gpio0__set_as_output(2, 0);
    gpio0__set_high(2, 0);
  }
}

/////////////////////PART 3 EC
void write_enable(void) {
  adesto_chip_select(1);                    // selects flash memory
  (void)ssp2__exchange_byte_personal(0x06); // Table 6-1 Write Enable
  adesto_chip_select(0);                    // deselects flash memory
}

void write_disable(void) {
  adesto_chip_select(1);
  (void)ssp2__exchange_byte_personal(0x04); // Table 6-1 Write Disable
  adesto_chip_select(0);
}

void adesto_read_at_page_address(uint32_t address, uint8_t *output) {
  adesto_chip_select(1); // select device

  ssp2__exchange_byte_personal(0x03); // OPcode to read (0Bh works also)
  adesto_flash_send_address(address); // send address of page to read
  for (int i = 0; i <= 255; i++) {
    output[i] = ssp2__exchange_byte_personal(0xFF); // sends dummy code, reads
  }
  adesto_chip_select(0);
}

void page_erase_set_high(uint32_t address) {
  // to modify page such as erase, must enable write
  write_enable();

  adesto_chip_select(1);
  /*  Page erase:
   *  Four Steps occur:
   *   1) OPCODE of 81 to erase
   *   2) Send 1 byte of dummy code
   *   3) send address of page to erase
   *   4) 1 byte of dummy code
   */
  ssp2__exchange_byte_personal(0x81);
  ssp2__exchange_byte_personal(0xC4);
  adesto_flash_send_address(address);
  ssp2__exchange_byte_personal(0xB2);
  adesto_chip_select(0);

  write_disable();
}

void adesto_write_at_page_address(uint32_t address, int direction_input) {
  write_enable(); // write enable on
  adesto_chip_select(1);
  ssp2__exchange_byte_personal(0x02); // opcode to select byte/page
  adesto_flash_send_address(address); // Page address

  if (direction_input == 0) { // to write onto page from 0xFF --> 0x00
    uint8_t page_input = 0xFF;
    for (int i = 0; i <= 255; i++) {
      ssp2__exchange_byte_personal(page_input);
      page_input--;
    }

  } else if (direction_input == 1) { // to write onto page from 0x00 --> 0xFF
    uint8_t page_input = 0x00;
    for (int i = 0; i <= 255; i++) {
      ssp2__exchange_byte_personal(page_input);
      page_input++;
    }
  }
  adesto_chip_select(0);
  write_disable();
}

// #include <stdbool.h>
// #include <stddef.h>

// #include "ssp2.h"

// #include "clock.h"
// #include "lpc40xx.h"
// #include "lpc_peripherals.h"

// uint8_t ssp2__exchange_byte(uint8_t byte_to_transmit) {
//   LPC_SSP0->DR = byte_to_transmit;

//   while (LPC_SSP0->SR & (1 << 4)) {
//     ; // Wait until SSP is busy
//   }

//   return (uint8_t)(LPC_SSP2->DR & 0xFF);
// }

// void ssp2__initialize(uint32_t max_clock_khz) {
//   lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);

//   LPC_SSP2->CR0 = 7;        // 8-bit mode
//   LPC_SSP2->CR1 = (1 << 1); // Enable SSP as Master
//   ssp2__set_max_clock(max_clock_khz);

//   ssp2__dma_init();
// }

// void ssp2__set_max_clock(uint32_t max_clock_khz) {
//   uint8_t divider = 2;
//   const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;

//   // Keep scaling down divider until calculated is higher
//   while (max_clock_khz < (cpu_clock_khz / divider) && divider <= 254) {
//     divider += 2;
//   }

//   LPC_SSP2->CPSR = divider;
// }

// /**
//  * These are the DMA request numbers that the hardware maps
//  * See LPC40xx user manual, chapter: General Purpose DMA controller, Table 696: DMA connections
//  */
// #define SSP2__DMA_REQUEST_TX 6UL
// #define SSP2__DMA_REQUEST_RX 7UL

// /**
//  * There are only 8 DMA channels on the LPC40xx so confirm validity
//  */
// #if !(SSP2__DMA_TX_CHANNEL >= 0 && SSP2__DMA_TX_CHANNEL <= 7)
// #error "SSP2__DMA_TX_CHANNEL must be between 0 and 7"
// #endif
// #if !(SSP2__DMA_RX_CHANNEL >= 0 && SSP2__DMA_RX_CHANNEL <= 7)
// #error "SSP2__DMA_RX_CHANNEL must be between 0 and 7"
// #endif

// typedef enum {
//   SSP_DMA__ERROR_NONE = 0,
//   SSP_DMA__ERROR_LENGTH = 1,
//   SSP_DMA__ERROR_BUSY = 2,
// } ssp_dma_error_e;

// /**
//  * Initialize the DMA struct pointers that we will use
//  */
// static const size_t ssp2__dma_channel_memory_spacing = (LPC_GPDMACH1_BASE - LPC_GPDMACH0_BASE);
// static LPC_GPDMACH_TypeDef *ssp2__dma_tx =
//     (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + (SSP2__DMA_TX_CHANNEL * ssp2__dma_channel_memory_spacing));
// static LPC_GPDMACH_TypeDef *ssp2__dma_rx =
//     (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + (SSP2__DMA_RX_CHANNEL * ssp2__dma_channel_memory_spacing));

// static void ssp2__dma_init(void);
// static ssp_dma_error_e ssp2__dma_transfer_block(unsigned char *buffer_pointer, uint32_t num_bytes, bool is_write_op);

// /*******************************************************************************
//  *
//  *                      P U B L I C    F U N C T I O N S
//  *
//  ******************************************************************************/

// // void ssp2__initialize(uint32_t max_clock_khz) {
// //   lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);

// //   LPC_SSP2->CR0 = 7;        // 8-bit mode
// //   LPC_SSP2->CR1 = (1 << 1); // Enable SSP as Master
// //   ssp2__set_max_clock(max_clock_khz);

// //   ssp2__dma_init();
// // }

// // void ssp2__set_max_clock(uint32_t max_clock_khz) {
// //   uint8_t divider = 2;
// //   const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;

// //   // Keep scaling down divider until calculated is higher
// //   while (max_clock_khz < (cpu_clock_khz / divider) && divider <= 254) {
// //     divider += 2;
// //   }

// //   LPC_SSP2->CPSR = divider;
// // }

// // uint8_t ssp2__exchange_byte(uint8_t byte_to_transmit) {
// //   LPC_SSP2->DR = byte_to_transmit;

// //   while (LPC_SSP2->SR & (1 << 4)) {
// //     ; // Wait until SSP is busy
// //   }

// void ssp2__dma_write_block(const unsigned char *output_block, size_t number_of_bytes) {
//   const bool is_write_operation = true;
//   ssp2__dma_transfer_block((unsigned char *)output_block, number_of_bytes, is_write_operation);
// }

// void ssp2__dma_read_block(unsigned char *input_block, size_t number_of_bytes) {
//   const bool is_write_operation = true;
//   ssp2__dma_transfer_block(input_block, number_of_bytes, !is_write_operation);
// }

// void ssp2__dma_init(void) {
//   lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__GPDMA);

//   const uint32_t enable_bitmask = (1 << 0);
//   LPC_GPDMA->Config = enable_bitmask;
//   while (!(LPC_GPDMA->Config & enable_bitmask)) {
//     ; // Wait for DMA to be enabled
//   }

//   // SSP dma channels will not use linked list transfers
//   ssp2__dma_rx->CLLI = 0;
//   ssp2__dma_tx->CLLI = 0;

//   // These registers are only initialized once, but this optimization makes no measurable difference
//   // ssp2__dma_tx->CDestAddr = (uint32_t)(&(LPC_SSP2->DR));
//   // ssp2__dma_rx->CSrcAddr = (uint32_t)(&(LPC_SSP2->DR));
// }

// ssp_dma_error_e ssp2__dma_transfer_block(unsigned char *buffer_pointer, uint32_t num_bytes, bool is_write_op) {
//   uint32_t dummyBuffer = 0xffffffff;

//   // DMA is limited to 12-bit transfer size
//   if (num_bytes >= 0x1000) {
//     return SSP_DMA__ERROR_LENGTH;
//   }

//   // DMA channels should not be busy
//   if ((ssp2__dma_tx->CConfig & 1) || (ssp2__dma_rx->CConfig & 1)) {
//     return SSP_DMA__ERROR_BUSY;
//   }

//   /**
//    * TO DO : Optimize SSP1 DMA
//    *  - Try setting source and destination burst size to 4
//    *  - Try setting 16-bit SPI, and source and destination width to 1 WORD
//    *
//    * LPC_SSP2->CR0 : B3:B0. 0b0111 = 8-bit and 0b1111 = 16-bit
//    * LPC_SSP2->CR0 |=  (1<<3);  // 16-bit
//    * LPC_SSP2->CR0 &= ~(1<<3);  // 8-bit
//    *
//    * Bits of DMACCControl:
//    * Transfer size: B11:B0
//    * Source Burst Size: B14:B12   - 0:1, 1:4, 2:8, 3:16 bytes
//    * Dest.  Burst Size: B17:B15   - 0:1, 1:4, 2:8, 3:16 bytes
//    * Source Width: B20:B18        - 0:Byte, 1:Word, 2:DWORD
//    * Dest.  Width: B23:B21        - 0:Byte, 1:Word, 2:DWORD
//    * Source increment: B26
//    * Destination increment: B27
//    * Terminal count interrupt enable: B31
//    *
//    * Bits of DMACCConfig:
//    * Enable: B0
//    * Source Peripheral: B5:B1   (ignored if source is memory)
//    * Dest.  Peripheral: B10:B6  (ignored if dest. is memory)
//    * Transfer type: B13:B11 - See below
//    * Interrupt Error Mask: B14
//    * Terminal Count Interrupt : B15
//    *
//    * Bits for transfer type:
//    * 000 - Memory to Memory
//    * 001 - Memory to Peripheral
//    * 010 - Peripheral to Memory
//    * 011 - Source peripheral to destination peripheral
//    */
//   static const uint32_t increment_source_address = UINT32_C(1) << 26;
//   static const uint32_t increment_destination_address = UINT32_C(1) << 27;

//   static const uint32_t transfer_memory_to_peripheral = UINT32_C(1) << 11;
//   static const uint32_t transfer_peripheral_to_memory = UINT32_C(2) << 11;
//   static const uint32_t dma_enable = UINT32_C(1) << 0;

//   static const uint32_t terminal_count_interrupt_enable = UINT32_C(1) << 31;

//   /**
//    * Clear existing terminal count and error interrupts otherwise
//    * DMA will not start.
//    */
//   LPC_GPDMA->IntTCClear = (1 << SSP2__DMA_RX_CHANNEL) | (1 << SSP2__DMA_TX_CHANNEL);
//   LPC_GPDMA->IntErrClr = (1 << SSP2__DMA_RX_CHANNEL) | (1 << SSP2__DMA_TX_CHANNEL);

//   /**
//    * From SPI to buffer:
//    * For write operation :
//    *      - Receive data into dummy buffer
//    *      - Don't increment destination
//    * For read operation:
//    *      - Read data into buffer_pointer
//    *      - Increment destination
//    *
//    * Note: Setting destination burst of 2 or 4 bytes makes no difference (1 << 15)
//    */
//   ssp2__dma_rx->CSrcAddr = (uint32_t)(&(LPC_SSP2->DR));
//   if (is_write_op) {
//     ssp2__dma_rx->CDestAddr = (uint32_t)(&dummyBuffer);
//     ssp2__dma_rx->CControl = num_bytes | terminal_count_interrupt_enable;
//   } else {
//     ssp2__dma_rx->CDestAddr = (uint32_t)buffer_pointer;
//     ssp2__dma_rx->CControl = num_bytes | increment_destination_address | terminal_count_interrupt_enable;
//   }
//   ssp2__dma_rx->CConfig = (SSP2__DMA_REQUEST_RX << 1) | transfer_peripheral_to_memory | dma_enable;

//   /**
//    * From buffer to SPI :
//    * For write operation :
//    *      - Source data is buffer_pointer
//    *      - Increment source data
//    * For read operation:
//    *      - Source data is buffer with 0xFF
//    *      - Don't increment source data
//    */
//   if (is_write_op) {
//     ssp2__dma_tx->CSrcAddr = (uint32_t)(buffer_pointer);
//     ssp2__dma_tx->CControl = num_bytes | increment_source_address;
//   } else {
//     ssp2__dma_tx->CSrcAddr = (uint32_t)(&dummyBuffer);
//     ssp2__dma_tx->CControl = num_bytes;
//   }
//   ssp2__dma_tx->CDestAddr = (uint32_t)(&(LPC_SSP2->DR));
//   ssp2__dma_tx->CConfig = (SSP2__DMA_REQUEST_TX << 6) | transfer_memory_to_peripheral | dma_enable;

//   /**
//    * Channel must be fully configured and then enabled separately.
//    * Setting DMACR's Rx/Tx bits should trigger the DMA
//    */
//   static const uint32_t enable_rx_tx_dma_triggers = 0x03;
//   LPC_SSP2->DMACR |= enable_rx_tx_dma_triggers;

//   while ((ssp2__dma_rx->CControl & 0xfff)) {
//     ; // Poll for DMA transfer to complete
//   }
//   LPC_SSP2->DMACR &= ~enable_rx_tx_dma_triggers;

//   return SSP_DMA__ERROR_NONE;
// }

// /*   Lab 6   */
// void ssp2__init(uint32_t max_clock_mhz) {
//   // Refer to LPC User manual and setup the register bits correctly (Table 482)
//   /// a) Power on Peripheral
//   LPC_SC->PCONP |= (1 << 20); // Table 16 bit 20, PCONP to power peripheral specifically

//   /// b) Setup control registers CR0 and CR1
//   LPC_SSP2->CR0 &= ~((1 << 5) | (1 << 4)); // Table 483 bit 5:4, 00 for SPI
//   LPC_SSP2->CR1 |= (1 << 1);               // Table 484 bit 1, SSP controller "enabled"

//   /// c) Setup prescalar register to be <= max_clock_mhz
//   LPC_SSP2->CPSR = max_clock_mhz; // Clock Prescale Register
// }

// uint8_t ssp2__exchange_byte_personal(uint8_t data_out) {
//   // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
//   LPC_SSP2->DR = data_out;
//   // Table 486, bit 4 (0 if idle, 1 if send/recieve)
//   while (LPC_SSP2->SR & (1 << 4)) { // while bit is 1
//     ;
//   }
//   return ((uint8_t)(LPC_SSP2->DR & 0xFF));
// }

// // configure port and pin numbers for ssp
// void ssp2_configure_flash() {
//   // table 84 funct 100 for sck, miso, mosi
//   // gpio__construct_with_function(0, 0, GPIO__FUNCTION_4);
//   // gpio__construct_with_function(0, 1, GPIO__FUNCTION_4);
//   // gpio__construct_with_function(0, 4, GPIO__FUNCTION_4);
//   LPC_IOCON->P1_0 &= ~0b111; // SCK2
//   LPC_IOCON->P1_1 &= ~0b111; // MOSI2
//   LPC_IOCON->P1_4 &= ~0b111; // MISO2

//   LPC_IOCON->P1_0 |= 0b100;
//   LPC_IOCON->P1_1 |= 0b100;
//   LPC_IOCON->P1_4 |= 0b100;
// }

// /**
//  * Adesto flash asks to send 24-bit address
//  * We can use our usual uint32_t to store the address
//  * and then transmit this address over the SPI driver
//  * one byte at a time
//  */
// void adesto_flash_send_address(uint32_t address) {
//   (void)ssp2__exchange_byte((address >> 16) & 0xFF);
//   (void)ssp2__exchange_byte((address >> 8) & 0xFF);
//   (void)ssp2__exchange_byte((address >> 0) & 0xFF);
// }

// // void adesto_chip_select(int state) {
// //   if (state == 1) {
// //     gpio0__set_as_output(1, 10);
// //     gpio0__set_low(1, 10);
// //     gpio0__set_as_output(2, 0);
// //     gpio0__set_low(2, 0);
// //   } else if (state == 0) {
// //     gpio0__set_as_output(1, 10);
// //     gpio0__set_high(1, 10);
// //     gpio0__set_as_output(2, 0);
// //     gpio0__set_high(2, 0);
// //   }
// // }

// /////////////////////PART 3 EC
// void write_enable(void) {
//   adesto_chip_select(1);                    // selects flash memory
//   (void)ssp2__exchange_byte_personal(0x06); // Table 6-1 Write Enable
//   adesto_chip_select(0);                    // deselects flash memory
// }

// void write_disable(void) {
//   adesto_chip_select(1);
//   (void)ssp2__exchange_byte_personal(0x04); // Table 6-1 Write Disable
//   adesto_chip_select(0);
// }

// void adesto_read_at_page_address(uint32_t address, uint8_t *output) {
//   adesto_chip_select(1); // select device

//   ssp2__exchange_byte_personal(0x03); // OPcode to read (0Bh works also)
//   adesto_flash_send_address(address); // send address of page to read
//   for (int i = 0; i <= 255; i++) {
//     output[i] = ssp2__exchange_byte_personal(0xFF); // sends dummy code, reads
//   }
//   adesto_chip_select(0);
// }

// void page_erase_set_high(uint32_t address) {
//   // to modify page such as erase, must enable write
//   write_enable();

//   adesto_chip_select(1);
//   /*  Page erase:
//    *  Four Steps occur:
//    *   1) OPCODE of 81 to erase
//    *   2) Send 1 byte of dummy code
//    *   3) send address of page to erase
//    *   4) 1 byte of dummy code
//    */
//   ssp2__exchange_byte_personal(0x81);
//   ssp2__exchange_byte_personal(0xC4);
//   adesto_flash_send_address(address);
//   ssp2__exchange_byte_personal(0xB2);
//   adesto_chip_select(0);

//   write_disable();
// }

// void adesto_write_at_page_address(uint32_t address, int direction_input) {
//   write_enable(); // write enable on
//   adesto_chip_select(1);
//   ssp2__exchange_byte_personal(0x02); // opcode to select byte/page
//   adesto_flash_send_address(address); // Page address

//   if (direction_input == 0) { // to write onto page from 0xFF --> 0x00
//     uint8_t page_input = 0xFF;
//     for (int i = 0; i <= 255; i++) {
//       ssp2__exchange_byte_personal(page_input);
//       page_input--;
//     }

//   } else if (direction_input == 1) { // to write onto page from 0x00 --> 0xFF
//     uint8_t page_input = 0x00;
//     for (int i = 0; i <= 255; i++) {
//       ssp2__exchange_byte_personal(page_input);
//       page_input++;
//     }
//   }
//   adesto_chip_select(0);
//   write_disable();
// }
