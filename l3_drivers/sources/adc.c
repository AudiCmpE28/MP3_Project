#include <stdint.h>

#include "adc.h"
#include "pwm1.h"

#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void adc__initialize(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__ADC);

  const uint32_t enable_adc_mask = (1 << 21);
  LPC_ADC->CR = enable_adc_mask;

  const uint32_t max_adc_clock = (12 * 1000UL * 1000UL); // 12.4Mhz : max ADC clock in datasheet for lpc40xx
  const uint32_t adc_clock = clock__get_peripheral_clock_hz();

  // APB clock divicer to support max ADC clock
  for (uint32_t divider = 2; divider < 255; divider += 2) {
    if ((adc_clock / divider) < max_adc_clock) {
      LPC_ADC->CR |= (divider << 8);
      break;
    }
  }
}

uint16_t adc__get_adc_value(adc_channel_e channel_num) {
  uint16_t result = 0;
  const uint16_t twelve_bits = 0x0FFF;
  const uint32_t channel_masks = 0xFF;
  const uint32_t start_conversion = (1 << 24);
  const uint32_t start_conversion_mask = (7 << 24); // 3bits - B26:B25:B24
  const uint32_t adc_conversion_complete = (1 << 31);

  if ((ADC__CHANNEL_2 == channel_num) || (ADC__CHANNEL_4 == channel_num) || (ADC__CHANNEL_5 == channel_num)) {
    LPC_ADC->CR &= ~(channel_masks | start_conversion_mask);
    // Set the channel number and start the conversion now
    LPC_ADC->CR |= (1 << channel_num) | start_conversion;

    while (!(LPC_ADC->GDR & adc_conversion_complete)) { // Wait till conversion is complete
      ;
    }
    result = (LPC_ADC->GDR >> 4) & twelve_bits; // 12bits - B15:B4
  }

  return result;
}

///////////////////////////////////////////////* Lab 5 part 1*/

void adc__enable_burst_mode(adc_channel_e channel_num) {
  LPC_ADC->CR &= ~(7 << 24);
  // setting channel of our choice
  LPC_ADC->CR |= (1 << channel_num);

  // bit 16 to enable burst
  LPC_ADC->CR |= (1 << 16);
} // LPC_ADC->CR |= ((1 << 16) | (1 << channel_num));

uint16_t adc__get_channel_reading_with_burst_mode(adc_channel_e channel_num) {
  // channel data register
  return ((LPC_ADC->DR[channel_num] >> 4) & 0x0FFF);
}

void ADC_enable_channel_pin(uint32_t port_number, uint32_t pin_number) {
  if (port_number == 0 && pin_number == 25) {
    LPC_IOCON->P0_25 &= ~(1 << 7); // sets it to analog mode (clear)
    gpio__construct_with_function(port_number, pin_number, 1);
    // LPC_IOCON->P0_25 &= ~(0b111); ^
    // LPC_IOCON->P0_25 |= 0b011;

  } else if (port_number == 1 && pin_number == 30) {
    LPC_IOCON->P1_30 &= ~(1 << 7);
    gpio__construct_with_function(port_number, pin_number, 3);

  } else if (port_number == 1 && pin_number == 31) {
    LPC_IOCON->P1_31 &= ~(1 << 7);
    gpio__construct_with_function(port_number, pin_number, 3);
  }
}
