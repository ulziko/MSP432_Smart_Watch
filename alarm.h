/*
 * alarm.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Dell
 */

#ifndef ALARM_H_
#define ALARM_H_

#include <stdint.h>
#include <ti/grlib/grlib.h>

void alarm_adc(uint64_t status, uint16_t *conversionValues);
void alarm_task(Graphics_Context *ctx);
void alarm_ta1_handler(void);
void alarm_button1_handler(void);   // S1 (P5.1)
void alarm_button2_handler(void);   // S2 (P3.5)

#endif /* ALARM_H_ */
