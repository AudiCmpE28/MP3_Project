/* MP3 FUNCTIONS (.h) */
#include "FreeRTOS.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

#include "acceleration.h"
#include "adc.h"
#include "board_io.h"
#include "cli_handlers.h"
#include "common_macros.h"
#include "delay.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "i2c.h"

#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "sl_string.h"
#include "ssp2.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 ========================================
*/

typedef void (*void_function_t)(void);
volatile bool play_STOP_semaphore;
volatile bool another_SONG;
volatile int volume_selection;
volatile int starter_music_silence;
volatile int song_number_browser;

volatile size_t song_number;
SemaphoreHandle_t PLAY_song;

typedef char songname_t[16];
typedef char songdata_t[512];

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

TaskHandle_t mp3_player_handle;

//-----------------------------------------------------------------------------------------------------
//***************************************|   MP3 DECODER  |*******************************************
//-----------------------------------------------------------------------------------------------------
void decoder__initialize(void);

void sj2_to_decoder_play_music(char data);

void sj2_WRITE_to_decoder(uint8_t address, uint16_t data);

uint16_t sj2_READ_to_decoder(uint8_t address);

//-----------------------------------------------------------------------------------------------------
//*******************************************|   MP3 FUNCTIONS  |**************************************
//-----------------------------------------------------------------------------------------------------
void volume_initialize(void);

//-----------------------------------*************************************------------------------------

int samples_from_acceleration_with_average_x(int *samples_of_100, acceleration__axis_data_s acc_object);

//-----------------------------------*************************************------------------------------
static songname_t list_of_songs[32];
static size_t number_of_songs;

static void song_list__handle_filename(const char *filename);

void song_list__populate(void);

size_t song_list__get_item_count(void);

const char *song_list__get_name_for_item(size_t item_number);

//-----------------------------------*************************************------------------------------
void interrupt_init(void);

void play_isr(void);

void next_song_isr(void);

void back_song_isr(void);

void volume_adjust(void);

void gpio_interrupt(void);

//-----------------------------------*************************************------------------------------
void volume_BASS_Treble_control(void);

//-----------------------------------------------------------------------------------------------------
//*******************************************|   MP3 LCD  |*******************************************
//-----------------------------------------------------------------------------------------------------
void lcd_initialize(void);

void lcd_print_screen(char *string_song, int volume_num);

void lcd_print_screen_next_song(char *string_song);