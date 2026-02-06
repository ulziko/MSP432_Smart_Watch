/*
 * tasks.c
 *
 *  Created on: 18 dic 2025
 *      Author: Lenovo
 */

#include <ti/grlib/grlib.h>
#include "tasks.h"
#include "main_page.h"
#include "activity_tracker.h"

tasks_t current_task = TIME_DISPLAY;

const t_task handlers[] = {
     {main_page_task, main_page_adc, no_op_task, no_op_task, main_page_button, main_page_button, no_op_task, "Main Page", MAIN_PAGE},
     {time_display, no_op_adc_task, no_op_task, time_display_ta1_handler , no_op_task, no_op_task, no_op_task, "Time Display", TIME_DISPLAY},
     {alarm_task, alarm_adc, no_op_task,  no_op_task, alarm_button1_handler, alarm_button2_handler, no_op_task, "Alarm", ALARM},
     {game_task, no_op_adc_task, game_ta0_handler,  no_op_task, game_button1_handler, game_button2_handler, no_op_task, "Game", GAME},
     {step_counter_task, activity_tracker_adc_handler, no_op_task, activity_tracker_timer_tick, no_op_task, no_op_task, no_op_task, "Steps", STEP_COUNTER},
};

void no_op_adc_task(uint64_t status, uint16_t* conversionValues) {

}
void no_op_task() {

}





/*
* Compile time error, declare an array of size -1 if the task count doesn't match the dimension of the array of tasks
*/

//If you get an error on the line below, make sure that thetask_handlers array matches the tasks in task_t.
typedef char task_count_doesnt_matchtask_flag_count[(TASK_COUNT == DIM(handlers)) - 1];



