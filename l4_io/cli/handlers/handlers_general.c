#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "cli_handlers.h"
#include "queue.h"
#include "sl_string.h"
#include "task.h"
#include "uart_printf.h"

static void cli__task_list_print(sl_string_t user_input_minus_command_name, app_cli__print_string_function cli_output);

/*/////////////////////////////////////////////////////////////////////////////////////////
                                  MP3 CLI Handler
////////////////////////////////////////////////////////////////////////////////////// */
// CLI needs access to the QueueHandle_t where you can queue the song name
// One way to do this is to declare 'QueueHandle_t' in main() that is NOT static
// and then extern it here

extern QueueHandle_t Q_songname;
app_cli_status_e cli__mp3_play(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output) {
  // sl_string__erase(user_input_minus_command_name, "play");
  xQueueSend(Q_songname, user_input_minus_command_name, portMAX_DELAY);

  return APP_CLI_STATUS__SUCCESS;
}

// app_cli_status_e cli__mp3_play(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
//                                app_cli__print_string_function cli_output) {
//   // music_song_t song_name;

//   // user_input_minus_command_name is actually a 'char *' pointer type
//   // We tell the Queue to copy 32 bytes of songname from this location
//   if (sl_string__begins_with_ignore_case(user_input_minus_command_name, "play")) {
//     (void)sl_string__erase_first_word(user_input_minus_command_name, ' ');
//     strncpy(song_name, user_input_minus_command_name, 128);
//     xQueueSend(Q_songname, song_name, portMAX_DELAY);
//   }

//   return APP_CLI_STATUS__SUCCESS;
// }

/*/////////////////////////////////////////////////////////////////////////////////////////
                                  Watchdog SD CLI
  ////////////////////////////////////////////////////////////////////////////////////// */
app_cli_status_e cli__watchdog_transmission(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                            app_cli__print_string_function cli_output) {

  // sl_string_t user_input_from_cli = user_input_minus_command_name;

  if (sl_string__begins_with_ignore_case(user_input_minus_command_name, "suspend")) {
    sl_string__erase_first_word(user_input_minus_command_name, ' ');
    TaskHandle_t handler_obtain_from_task = xTaskGetHandle(user_input_minus_command_name);

    if (handler_obtain_from_task != NULL) {
      vTaskSuspend(handler_obtain_from_task);
    } else {
      sl_string__insert_at(user_input_minus_command_name, 0, "Unknown Task (suspend): ");
      cli_output(NULL, user_input_minus_command_name);
    }
    printf("\n");

  } else if (sl_string__begins_with_ignore_case(user_input_minus_command_name, "resume")) {
    sl_string__erase_first_word(user_input_minus_command_name, ' ');
    TaskHandle_t handler_obtain_from_task = xTaskGetHandle(user_input_minus_command_name);

    if (handler_obtain_from_task != NULL) {
      vTaskResume(handler_obtain_from_task);

    } else {
      sl_string__insert_at(user_input_minus_command_name, 0, "Unknown Task (resume): ");
      cli_output(NULL, user_input_minus_command_name);
    }

  } else {
    cli_output(NULL, "ERROR: Invalid User Input[resume or suspend taskname]\n\n\n");
  }

  return APP_CLI_STATUS__SUCCESS;
}

app_cli_status_e cli__crash_me(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                               app_cli__print_string_function cli_output) {
  uint32_t *bad_pointer = (uint32_t *)0x00000001;
  *bad_pointer = 0xDEADBEEF;
  return APP_CLI_STATUS__SUCCESS;
}

app_cli_status_e cli__task_list(app_cli__argument_t argument, sl_string_t user_input_minus_command_name,
                                app_cli__print_string_function cli_output) {
  const int sleep_time = sl_string__to_int(user_input_minus_command_name);
  if (sleep_time > 0) {
    vTaskResetRunTimeStats();
    vTaskDelay(sleep_time);
  }

  // re-use user_input_minus_command_name as 'output_string' to save memory:
  sl_string_t output_string = user_input_minus_command_name;
  cli__task_list_print(output_string, cli_output);

  return APP_CLI_STATUS__SUCCESS;
}

static void cli__task_list_print(sl_string_t output_string, app_cli__print_string_function cli_output) {
  void *unused_cli_param = NULL;

#if (0 != configUSE_TRACE_FACILITY)
  // Enum to char : eRunning, eReady, eBlocked, eSuspended, eDeleted
  static const char *const task_status_table[] = {"running", " ready ", "blocked", "suspend", "deleted"};

  // Limit the tasks to avoid heap allocation.
  const unsigned portBASE_TYPE max_tasks = 10;
  TaskStatus_t status[max_tasks];
  uint32_t total_cpu_runtime = 0;
  uint32_t total_tasks_runtime = 0;

  const uint32_t total_run_time = portGET_RUN_TIME_COUNTER_VALUE();
  const unsigned portBASE_TYPE task_count = uxTaskGetSystemState(&status[0], max_tasks, &total_cpu_runtime);

  sl_string__printf(output_string, "%10s  Status Pr Stack CPU%%          Time\n", "Name");
  cli_output(unused_cli_param, output_string);

  for (unsigned priority_number = 0; priority_number < configMAX_PRIORITIES; priority_number++) {
    /* Print in sorted priority order */
    for (unsigned i = 0; i < task_count; i++) {
      const TaskStatus_t *task = &status[i];
      if (task->uxBasePriority == priority_number) {
        total_tasks_runtime += task->ulRunTimeCounter;

        const unsigned cpu_percent = (0 == total_cpu_runtime) ? 0 : task->ulRunTimeCounter / (total_cpu_runtime / 100);
        const unsigned time_us = task->ulRunTimeCounter;
        const unsigned stack_in_bytes = (sizeof(void *) * task->usStackHighWaterMark);

        sl_string__printf(output_string, "%10s %s %2u %5u %4u %10u us\n", task->pcTaskName,
                          task_status_table[task->eCurrentState], (unsigned)task->uxBasePriority, stack_in_bytes,
                          cpu_percent, time_us);
        cli_output(unused_cli_param, output_string);
      }
    }
  }

  sl_string__printf(output_string, "Overhead: %u uS\n", (unsigned)(total_run_time - total_tasks_runtime));
  cli_output(unused_cli_param, output_string);
#else
  cli_output(unused_cli_param, "Unable to provide you the task information along with their CPU and stack usage.\n");
  cli_output(unused_cli_param, "configUSE_TRACE_FACILITY macro at FreeRTOSConfig.h must be non-zero\n");
#endif
}
