/* MP3 FUNCTIONS (.c) */
#include "mp3_functions.h"

//-----------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 VOLUME  |-------------------------------------------
//-----------------------------------------------------------------------------------------------------
void volume_initialize(void) {
  // Analog Digital Converter (ADC)
  adc__initialize();
  adc__enable_burst_mode(ADC__CHANNEL_4);
  ADC_enable_channel_pin(1, 30);
}

//-----------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 Accelerator  |-------------------------------------------
//-----------------------------------------------------------------------------------------------------
int samples_from_acceleration_with_average_x(int *samples_of_100, acceleration__axis_data_s acc_object) {
  int i = 0;
  int sensor_value = 0;
  while (i < 100) {
    acc_object = acceleration__get_data();
    samples_of_100[i] = acc_object.y;
    sensor_value = samples_of_100[i] + sensor_value;
    i++;
  }
  return (sensor_value / 100);
}

//-----------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 LIST  |-------------------------------------------
//-----------------------------------------------------------------------------------------------------
static void song_list__handle_filename(const char *filename) {
  // This will not work for cases like "file.mp3.zip"
  if (NULL != strstr(filename, ".mp3")) {
    printf("Filename: %s\n", filename);
    strncpy(list_of_songs[number_of_songs], filename, sizeof(songname_t) - 1);
    number_of_songs++;
  }
}

void song_list__populate(void) {
  FRESULT res;
  static FILINFO file_info;
  const char *root_path = "/";

  DIR dir;
  res = f_opendir(&dir, root_path);

  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &file_info); /* Read a directory item */
      if (res != FR_OK || file_info.fname[0] == 0) {
        break; /* Break on error or end of dir */
      }

      if (file_info.fattrib & AM_DIR) {
        /* Skip nested directories, only focus on MP3 songs at the root */
      } else { /* It is a file. */
        song_list__handle_filename(file_info.fname);
      }
    }
    f_closedir(&dir);
  }
}

size_t song_list__get_item_count(void) { return number_of_songs; }

const char *song_list__get_name_for_item(size_t item_number) {
  const char *return_pointer = "";

  if (item_number >= number_of_songs) {
    return_pointer = "";
  } else {
    return_pointer = list_of_songs[item_number];
  }

  return return_pointer;
}

//-----------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 Interrupts  |-------------------------------------------
//-----------------------------------------------------------------------------------------------------
void play_isr(void) { xSemaphoreGiveFromISR(PLAY_song, NULL); }

void next_song_isr(void) {
  if (song_number < song_list__get_item_count()) {
    song_number++;       // add more
    another_SONG = true; // and make thyis true
  }
}

void back_song_isr(void) {
  if (song_number > 0) {
    song_number--;
    another_SONG = true;
  }
}

void volume_adjust(void) {
  if (volume_selection == 0) {
    volume_selection = 1;
    fprintf(stderr, "SEL 1\n");
  } else {
    fprintf(stderr, "SEL 0\n");
    volume_selection = 0;
  }
}

// dispatcher for clearing
// so it can be ready for another input
void gpio_interrupt(void) { gpio0__interrupt_dispatcher(); }

// pins for Interupt
void interrupt_init(void) {
  gpio_input(2, 2);
  gpio_input(2, 5);
  gpio_input(2, 7);
  gpio_input(2, 9);
  // function pointer to access each function
  void_function_t isr_pins[] = {play_isr, next_song_isr, back_song_isr, volume_adjust};
  gpio0__attach_interrupt(2, 2, GPIO_INTR__RISING_EDGE, isr_pins[0]);
  gpio0__attach_interrupt(2, 5, GPIO_INTR__RISING_EDGE, isr_pins[1]);
  gpio0__attach_interrupt(2, 7, GPIO_INTR__RISING_EDGE, isr_pins[2]);
  gpio0__attach_interrupt(2, 9, GPIO_INTR__RISING_EDGE, isr_pins[3]);
  // add select specific song
  NVIC_EnableIRQ(GPIO_IRQn);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, NULL);
}

//-----------------------------------------------------------------------------------------------------
//--------------------------------|   MP3 Volume + BASS + Treble  |------------------------------------
//-----------------------------------------------------------------------------------------------------
void volume_BASS_Treble_control(void) {
  double adc_reading = 0;
  uint8_t volume_adjust;
  uint16_t volume;
  uint8_t settings_for_BASS;
  uint8_t bass_freq = 6;
  uint8_t settings_for_TREBLE;
  uint8_t treble_freq = 5;

  if (volume_selection == 0) { // VOLUME
    adc_reading = (((double)adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_4) / 4095) * 254);
    volume_adjust = ((uint8_t)(adc_reading)); // needed to cast because we have a double
    volume = volume_adjust;
    volume = (volume << 8);
    volume = volume | volume_adjust;
    sj2_WRITE_to_decoder(0xB, volume);

    adc_reading = (adc_reading / 254) * 100; // volume for display
    lcd_print_screen(song_list__get_name_for_item(song_number), adc_reading);

  } else if (volume_selection == 1) { // Playlist
    lcd_print_screen_next_song(song_list__get_name_for_item(song_number_browser));
  }
}