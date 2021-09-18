// The idea is that you will use interrupts to input data to FreeRTOS queue
// Then, instead of polling, your tasks can sleep on data that we can read from the queue
#include "FreeRTOS.h"
#include "queue.h"

#include "uart_lab.h"

// Private queue handle of our uart_lab.c
static QueueHandle_t your_uart_rx_queue;

void configure_uart2_pins(void) {
  LPC_IOCON->P2_8 &= ~0b111; // Clear P2.8
  LPC_IOCON->P2_9 &= ~0b111; // Clear P2.9
  LPC_IOCON->P2_8 |= 0b010;  // U2_TXD
  LPC_IOCON->P2_9 |= 0b010;  // U2_RXD
}

void configure_uart3_pins(void) {
  LPC_IOCON->P0_0 &= ~0b111; // Clear P0.0
  LPC_IOCON->P0_1 &= ~0b111; // Clear P0.1
  LPC_IOCON->P0_0 |= 0b010;  // U3_TXD
  LPC_IOCON->P0_1 |= 0b010;  // U3_RXD
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Refer to LPC User manual and setup the register bits correctly
  // The first page of the UART chapter has good instructions
  // a) Power on Peripheral
  // b) Setup DLL, DLM, FDR, LCR registers
  peripheral_clock = peripheral_clock * 1000000;
  const uint16_t uart_DL = peripheral_clock / (16 * baud_rate);

  if (uart == UART_2) { /* UART 2 */
    // lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART2);
    LPC_SC->PCONP |= (1 << 24); // Table 16 bit 24: Power On
    configure_uart2_pins();

    LPC_UART2->LCR |= (1 << 7); // Enable access to Divisor Latches
    LPC_UART2->LCR |= (3 << 0); // 8-bit character length  (0x3)

    LPC_UART2->DLL |= ((uart_DL >> 0) & 0xFF); // divider latch LSB ~least significant bit
    LPC_UART2->DLM |= ((uart_DL >> 8) & 0xFF); // divider latch MSB ~most sognificant bit
    LPC_UART2->FDR |= (0 << 0) | (1 << 4);     // divaddval | mulval

    LPC_UART2->LCR &= ~(1 << 7); // Disable access to Divisor Latches

  } else if (uart == UART_3) { /* UART 3 */
    // lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);
    LPC_SC->PCONP |= (1 << 25); // Table 16 bit 25: Power On
    configure_uart3_pins();

    LPC_UART3->LCR |= (1 << 7); // Enable access to Divisor Latches
    LPC_UART3->LCR |= (3 << 0); // 8-bit character length (0x3)

    LPC_UART3->DLL |= ((uart_DL >> 0) & 0xFF); // divider latch LSB ~least significant bit
    LPC_UART3->DLM |= ((uart_DL >> 8) & 0xFF); // divider latch MSB ~most sognificant bit
    LPC_UART3->FDR |= (0 << 0) | (1 << 4);     // divaddval | mulval

    LPC_UART3->LCR &= ~(1 << 7); // Disable access to Divisor Latches
  }
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte

  /*Table 393, DLAB = 0 for RBR to function */
  if (uart == UART_2) {
    LPC_UART2->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART2->LSR & (1 << 0))) { // Table 402: RDR Receiver Data ready
      ;                                    // while Reciever not empty
    }
    *input_byte = LPC_UART2->RBR & 0xFF; // copy RBR to pointer

  } else if (uart == UART_3) {
    LPC_UART3->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART3->LSR & (1 << 0))) { // Table 402: RDR Receiver Data ready
      ;
    }
    *input_byte = LPC_UART3->RBR & 0xFF; // copy RBR to pointer
  }

  if (*input_byte == 0) {
    return false;
  } else {
    return true;
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  // b) Copy output_byte to THR register

  if (uart == UART_2) {
    LPC_UART2->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART2->LSR & (1 << 5))) { // Table 402: THRE Transmitter holding reg empty
      ;                                    // while UnTHR contaoins valid data
    }
    LPC_UART2->THR = (output_byte >> 0); // Store output to THR

  } else if (uart == UART_3) {
    LPC_UART3->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART3->LSR & (1 << 5))) { // Table 402: THRE Transmitter holding reg empty
      ;                                    // while UnTHR contaoins valid data
    }
    LPC_UART3->THR = (output_byte >> 0); // Store output to THR
  }

  if (output_byte == 0) {
    return false;
  } else {
    return true;
  }
}

/***************************************************************************************/
//////////////  PART 2
/***************************************************************************************/

// Private function of our uart_lab.c
static void your_receive_interrupt_for_uart_2(void) {
  if (LPC_UART2->IIR & (2 << 0)) { // Table 398: Interrupt Identification (INTID)
    // Recieve data availabe (RDA)
    LPC_UART2->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART2->LSR & (1 << 0))) { // Table 402:~RDR reciever not empty
      ;                                    // while UnTHR contains valid data
    }
  }
  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  const char byte = LPC_UART2->RBR & 0xFF;
  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

static void your_receive_interrupt_for_uart3(void) {
  // Recieve data availabe (RDA)
  if (LPC_UART3->IIR & (2 << 0)) {
    LPC_UART3->LCR &= ~(1 << 7);           // Table 401: Disable Divisor latch
    while (!(LPC_UART3->LSR & (1 << 0))) { // Table 402:~RDR reciever not empty
      ;                                    // while UnTHR contains valid data
    }
  }
  const char byte = LPC_UART3->RBR & 0xFF;
  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number, uint16_t queue_Size) {
  // TODO: Create your RX queue
  your_uart_rx_queue = xQueueCreate(queue_Size, sizeof(char));

  if (uart_number == UART_2) {
    NVIC_EnableIRQ(UART2_IRQn);
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt_for_uart_2, NULL);

    LPC_UART2->LCR &= ~(1 << 7); // Table 401: Disable Divisor latch
    LPC_UART2->IER |= (1 << 0);  // Enable RDA interrupts

  } else if (uart_number == UART_3) {
    NVIC_EnableIRQ(UART3_IRQn);
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt_for_uart3, NULL);

    LPC_UART3->LCR &= ~(1 << 7); // Table 401: Disable Divisor latch
    LPC_UART3->IER |= (1 << 0);
  }
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}
