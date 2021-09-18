/* MP3 DECODER (.c) */ 
//by IZ da Wiz
#include "mp3_functions.h"

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
static void chip_selection(bool chip_select) {
  if (chip_select) {
    LPC_GPIO0->CLR = (1 << 26);
  } else {
    LPC_GPIO0->SET = (1 << 26);
  }
}

void sj2_WRITE_to_decoder(uint8_t address, uint16_t data) {
  chip_selection(true);
  ssp2__exchange_byte(0x2);
  ssp2__exchange_byte(address);
  ssp2__exchange_byte((data >> 8) & 0xFF);
  ssp2__exchange_byte((data >> 0) & 0xFF);
  chip_selection(false);
}

uint16_t sj2_READ_to_decoder(uint8_t address) {
  uint8_t temp1, temp2;
  chip_selection(true);
  ssp2__exchange_byte(0x3);
  ssp2__exchange_byte(address);
  temp1 = ssp2__exchange_byte(0xFF);
  temp2 = ssp2__exchange_byte(0xFF);
  chip_selection(false);

  uint16_t finalvalue = 0;
  finalvalue |= ((temp1 << 8) | (temp2 << 0));
  return finalvalue;
}

void sj2_to_decoder_play_music(char data) {
  LPC_GPIO1->CLR = (1 << 31); // gpio__reset(XDCS);
  ssp2__exchange_byte(data);
  LPC_GPIO1->SET = (1 << 31); // gpio__set(XDCS);
}

void decoder__initialize(void) {
  // Configure as SPI pins
  gpio__construct_with_function(0, 1, GPIO__FUNCTION_4); // SCK
  gpio__construct_with_function(1, 4, GPIO__FUNCTION_4); // MISO
  gpio__construct_with_function(1, 1, GPIO__FUNCTION_4); // MOSI

  /* DREQ */
  gpio_input(1, 20);
  /* CS */
  gpio_output(0, 26);
  /* XDCS */
  gpio_output(1, 31);
  /* RESET */
  gpio_output(0, 8);

  ssp2__initialize(1000);
  if (!gpio_level(0, 8)) {     // gpio__get(RST)
    LPC_GPIO0->CLR = (1 << 8); // gpio__reset(RST);
    delay__ms(100);
    LPC_GPIO0->SET = (1 << 8); // gpio__set(RST);
  }
  LPC_GPIO0->SET = (1 << 26); // gpio__set(CS);
  LPC_GPIO1->SET = (1 << 31); // gpio__set(XDCS);

  delay__ms(100);
  sj2_WRITE_to_decoder(0x0, 0x800 | 0x4); //(0x0, 0x800 | 0x4);
  delay__ms(200);
  sj2_WRITE_to_decoder(0x3, 0x6000);
}