/*
 * tasks.h
 *
 *  Created on: 20 nov 2025
 *      Author: Lenovo
 */

#ifndef TASKS_H_
#define TASKS_H_

#include <ti/grlib/grlib.h>

#include "time_display.h"
#include "activity_tracker.h"



#define DIM(x) (sizeof(x) / sizeof(*(x)))

typedef void (*task_handler_t) (Graphics_Context *g_sContext);
typedef void (*adc_task_handler_t) (uint64_t status, uint16_t* conversionValues);
// you can access to the system time by  global variable named LocalTime by including system_time.h header
typedef void (*timer_TA0_handler_t) ();

typedef struct {
    task_handler_t main_handler;
    adc_task_handler_t adc_handler;
    timer_TA0_handler_t ta0_handler;
}t_task;

void no_op_adc_task(uint64_t status, uint16_t* conversionValues) {}



const t_task handlers[] = {
       {time_display, no_op_adc_task, time_display_TA0_handler},
     //{alarm_task, alarm_adc, alarm_timer},
       {step_counter_task, step_counter_adc, step_counter_timer}
};



typedef enum {
    TIME_DISPLAY,
    STEP_COUNTER,
    TASK_COUNT
} tasks_t;

/*
* Compile time error, declare an array of size -1 if the task count doesn't match the dimension of the array of tasks
*/

//If you get an error on the line below, make sure that thetask_handlers array matches the tasks in task_t.
typedef char task_count_doesnt_matchtask_flag_count[(TASK_COUNT == DIM(handlers)) - 1];

#endif /* TASKS_H_ */
