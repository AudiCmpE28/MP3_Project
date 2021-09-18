/* MAIN (.c) */
#include "mp3_functions.h"

//---------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 Reader  |-------------------------------------------
//---------------------------------------------------------------------------------------------------
static void mp3_reader_task(void *p) {
  while (1) {
    songname_t name = {0};
    xQueueReceive(Q_songname, &name[0], portMAX_DELAY);

    FIL file;
    char buffer[512];

    UINT num_of_bytes_read = 0;
    FRESULT result = f_open(&file, name, FA_READ);

    if (FR_OK == result) {
      f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);

      while (num_of_bytes_read != 0) {
        f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
        xQueueSend(Q_songdata, buffer, portMAX_DELAY);
        num_of_bytes_read++;

        if (another_SONG) {
          num_of_bytes_read = 0;
          another_SONG = false;
        }
      }
      f_close(&file);

    } else {
      lcd_print_screen("File not opened", 0);
      delay__ms(4000);
    }
  }
}

//---------------------------------------------------------------------------------------------------
//--------------------------------------|   MP3 Player  |-------------------------------------------
//---------------------------------------------------------------------------------------------------
static void mp3_player_task(void *p) {
  songdata_t data_of_song;
  while (1) {
    if (xQueueReceive(Q_songdata, &data_of_song[0], portMAX_DELAY)) {
      for (size_t j = 0; j < sizeof(songdata_t); j++) {
        while (!gpio_level(1, 20)) {
          ; // DREQ :: is decoder busy?
        }
        sj2_to_decoder_play_music(data_of_song[j]);
        // song_sec++;
        // if (song_sec == 60) {
        //   song_min++;
        //   song_sec = 0;
        // }
      }
    }
  }
}

//-----------------------------------------------------------------------------------------------------
//------------------------------------|   MP3 Controller  |---------------------------------------
//-----------------------------------------------------------------------------------------------------
static void mp3_task_control(void *p) {
  size_t song_ATM = 1;
  songdata_t song_data_name;
  int sensor_value = 0, song_tracker_list = 0;
  int samples_of_100[100];
  acceleration__axis_data_s acceleration_object;

  while (1) {
    /* sends song name to mp3 player */
    if (song_number != song_ATM) { // sends first, next, back song to mp3 reader
      song_ATM = song_number;
      xQueueSend(Q_songname, song_list__get_name_for_item(song_number), portMAX_DELAY);
    }

    /* We can use tilt to skip song forward or backward */
    sensor_value = (samples_from_acceleration_with_average_x(samples_of_100, acceleration_object) / 100);

    if (sensor_value >= 1 && song_number_browser < (int)song_list__get_item_count()) {
      song_number_browser++; // add more

    } else if (sensor_value <= -1 && song_number_browser > 0) {
      song_number_browser--;
    }

    /* Play and Pause */
    if (xSemaphoreTake(PLAY_song, 1000)) {
      if (play_STOP_semaphore) {
        play_STOP_semaphore = false;
        vTaskSuspend(mp3_player_handle);
      } else {
        play_STOP_semaphore = true;
        vTaskResume(mp3_player_handle);
      }
    }

    volume_BASS_Treble_control(); // volume control
  }
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------|   MAIN Function + inits  |---------------------------------------
//-----------------------------------------------------------------------------------------------------
int main(void) {
  /* Initializations of external devices and SJ2 */
  sj2_cli__init();
  decoder__initialize();
  volume_initialize();
  interrupt_init();
  song_list__populate();
  acceleration__init();
  lcd_initialize();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));
  PLAY_song = xSemaphoreCreateBinary();

  /*volatile variables*/
  play_STOP_semaphore = false;
  another_SONG = false;
  volume_selection = 0;
  song_number = 0;

  /* 3 Tasks */
  xTaskCreate(mp3_reader_task, "mp3_reader", 4096 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "mp3_player", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, &mp3_player_handle);
  xTaskCreate(mp3_task_control, "mp3_control", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  vTaskStartScheduler();
}