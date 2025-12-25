/*
 * tasks.h
 *
 *  Created on: 20 nov 2025
 *      Author: Lenovo
 */

#ifndef TASKS_H_
#define TASKS_H_

#include <ti/grlib/grlib.h>
#include <stdint.h>

#include "main_page.h"
#include "time_display.h"
#include "alarm.h"

#define DIM(x) (sizeof(x) / sizeof(*(x)))

typedef void (*task_handler_t) (Graphics_Context *g_sContext);
typedef void (*adc_task_handler_t) (uint64_t status, uint16_t* conversionValues);
// you can access to the system time by  global variable named LocalTime by including system_time.h header
typedef void (*timer_TA0_handler_t) ();
typedef void (*button_handler_t) ();



typedef enum {
    MAIN_PAGE,
    TIME_DISPLAY,
    ALARM,
    TASK_COUNT
} tasks_t;


typedef struct {
    task_handler_t main_handler;      //Main thread
    adc_task_handler_t adc_handler;   //Adc handler (returns 3 values)
    timer_TA0_handler_t ta0_handler;  //Timer handler
    button_handler_t button1_handler; //Upper button handler (S1)
    button_handler_t button2_handler; //Lower button handler (S2)
    char task_name[15];
    tasks_t task_enum;

}t_task;

void no_op_adc_task(uint64_t status, uint16_t* conversionValues);
void no_op_task();


extern tasks_t current_task;
extern const t_task handlers[];


#endif /* TASKS_H_ */
