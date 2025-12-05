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
task_handler_t task_handlers[] = {
                                  time_display,
                                  activity_tracker
};


void no_op_adc_task(uint64_t status, uint16_t* conversionValues) {}


typedef void (*adc_task_handler_t) (uint64_t status, uint16_t* conversionValues);
adc_task_handler_t adc_task_handlers[] = {
                                  no_op_adc_task,
                                  activity_tracker_adc
};


typedef enum {
    TIME_DISPLAY,
    ACTIVITY_TRACKER,

    TASK_COUNT
} tasks_t;

/*
* Compile time error, declare an array of size -1 if the task count doesn't match the dimension of the array of tasks
*/

//If you get an error on the line below, make sure that thetask_handlers array matches the tasks in task_t.
typedef char task_count_doesnt_matchtask_flag_count[(TASK_COUNT == DIM(task_handlers) || TASK_COUNT == DIM(adc_task_handlers)) - 1];


#endif /* TASKS_H_ */
