/* MP3 LCD (.c) */
#include "mp3_functions.h"

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
static void selection_lcd(bool select) {
  if (select) {
    gpio_high(1, 29);
  } else {
    gpio_low(1, 29);
  }
}

static void RS_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(1, 23);
  } else if (active_l_h == 0) {
    gpio_low(1, 23);
  }
}

static void DB7_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(0, 0);
  } else if (active_l_h == 0) {
    gpio_low(0, 0);
  }
}

static void DB6_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(0, 22);
  } else if (active_l_h == 0) {
    gpio_low(0, 22);
  }
}

static void DB5_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(0, 17);
  } else if (active_l_h == 0) {
    gpio_low(0, 17);
  }
}

static void DB4_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(0, 16);
  } else if (active_l_h == 0) {
    gpio_low(0, 16);
  }
}

static void DB3_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(2, 8);
  } else if (active_l_h == 0) {
    gpio_low(2, 8);
  }
}

static void DB2_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(2, 6);
  } else if (active_l_h == 0) {
    gpio_low(2, 6);
  }
}

static void DB1_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(2, 4);
  } else if (active_l_h == 0) {
    gpio_low(2, 4);
  }
}

static void DB0_bit(int active_l_h) {
  if (active_l_h == 1) {
    gpio_high(2, 1);
  } else if (active_l_h == 0) {
    gpio_low(2, 1);
  }
}

void lcd_initialize(void) {
  // ** Write pin set to low since we will never read **
  // SET UP
  gpio_output(1, 23); // RS  [pin 4]
  gpio_output(1, 29); // E   [pin 6]
  gpio_output(2, 1);  // DB0 [pin 7]
  gpio_output(2, 4);  // DB1 [pin 8]
  gpio_output(2, 6);  // DB2 [pin 9]
  gpio_output(2, 8);  // DB3 [pin 10]
  gpio_output(0, 16); // DB4 [pin 11]
  gpio_output(0, 17); // DB5 [pin 12]
  gpio_output(0, 22); // DB6 [pin 13]
  gpio_output(0, 0);  // DB7 [pin 14]

  selection_lcd(false);
  RS_bit(0);  // 0
  DB7_bit(0); // 0
  DB6_bit(0); // 0
  DB5_bit(0); // 1
  DB4_bit(0); // 1
  DB3_bit(0); // x
  DB2_bit(0); // x
  DB1_bit(0); // x
  DB0_bit(0); // x
  delay__ms(50);

  // ~~ function set ~~
  /* 8 bit mode -- 1 line */
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // 0
  DB7_bit(0); // 8
  DB6_bit(0); // 4
  DB5_bit(1); // 2
  DB4_bit(1); // 1  :: 8 bit mode
  DB3_bit(0); // 8 :: 2 line HI, 1 line LO
  DB2_bit(0); // 4 :: 5x11
  DB1_bit(0); // 2
  DB0_bit(0); // 1
  delay__ms(1);
  selection_lcd(false);

  /* Clear Display */
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // RS
  DB7_bit(0); // 8
  DB6_bit(0); // 4
  DB5_bit(0); // 2
  DB4_bit(0); // 1
  DB3_bit(0); // 8
  DB2_bit(0); // 4
  DB1_bit(0); // 2
  DB0_bit(1); // 1
  delay__ms(1);
  selection_lcd(false);

  /* Return Home */
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // RS
  DB7_bit(0); // 8t
  DB6_bit(0); // 4
  DB5_bit(0); // 2
  DB4_bit(0); // 1
  DB3_bit(0); // 8
  DB2_bit(0); // 4
  DB1_bit(1); // 2
  DB0_bit(0); // 1
  delay__ms(1);
  selection_lcd(false);

  /* Display on, no cursor */
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // RS
  DB7_bit(0); // 8
  DB6_bit(0); // 4
  DB5_bit(0); // 2
  DB4_bit(0); // 1
  DB3_bit(1); // 8
  DB2_bit(1); // 4
  DB1_bit(0); // 2
  DB0_bit(0); // 1
  delay__ms(1);
  selection_lcd(false);
}

static void send_characters_lcd(uint8_t data) {
  selection_lcd(true);
  delay__ms(1);
  RS_bit(1); // RS

  if ((data >> 0) & 0x01) {
    DB0_bit(1);
  } else {
    DB0_bit(0);
  }

  if ((data >> 1) & 0x01) {
    DB1_bit(1);
  } else {
    DB1_bit(0);
  }

  if ((data >> 2) & 0x01) {
    DB2_bit(1);
  } else {
    DB2_bit(0);
  }

  if ((data >> 3) & 0x01) {
    DB3_bit(1);
  } else {
    DB3_bit(0);
  }

  if ((data >> 4) & 0x01) {
    DB4_bit(1);
  } else {
    DB4_bit(0);
  }

  if ((data >> 5) & 0x01) {
    DB5_bit(1);
  } else {
    DB5_bit(0);
  }

  if ((data >> 6) & 0x01) {
    DB6_bit(1);
  } else {
    DB6_bit(0);
  }

  if ((data >> 7) & 0x01) {
    DB7_bit(1);
  } else {
    DB7_bit(0);
  }

  delay__ms(1);
  selection_lcd(false);
}

static void clear_lcd_screen(void) {
  /* Clear Display */
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // RS
  DB7_bit(0); // 8
  DB6_bit(0); // 4
  DB5_bit(0); // 2
  DB4_bit(0); // 1
  DB3_bit(0); // 8
  DB2_bit(0); // 4
  DB1_bit(0); // 2
  DB0_bit(1); // 1
  delay__ms(1);
  selection_lcd(false);
}

static void lcd_2nd_row(void) {
  selection_lcd(true);
  delay__ms(1);
  RS_bit(0);  // RS
  DB7_bit(1); // 8
  DB6_bit(1); // AC6
  DB5_bit(0); // AC5
  DB4_bit(0); // AC4
  DB3_bit(0); // AC3
  DB2_bit(0); // AC2
  DB1_bit(0); // AC1
  DB0_bit(0); // AC0
  delay__ms(1);
  selection_lcd(false);
}

void lcd_print_screen(char *string_song, int volume_num) {
  /* First Row: Song[11 blocks] | Space [1 block] | volume [4 blocks] */
  clear_lcd_screen();
  char percentage[] = "%";
  char volume[4] = "";
  itoa(volume_num, volume, 10);
  strcat(volume, percentage);

  for (size_t i = 0; i < 11; i++) {
    if (i < strlen(string_song) - 4) {
      send_characters_lcd((uint8_t)string_song[i]);
    } else {
      send_characters_lcd(0x20); // space
    }
  }
  send_characters_lcd(0x20); // space
  for (size_t i = 0; i < strlen(volume); i++) {
    send_characters_lcd((uint8_t)volume[i]);
  }
}

void lcd_print_screen_next_song(char *string_song) {
  /* First Row: Song[11 blocks] | Space [1 block] | volume [4 blocks] */
  clear_lcd_screen();
  for (size_t i = 0; i < strlen(string_song) - 4; i++) {
    send_characters_lcd((uint8_t)string_song[i]);
  }
}
