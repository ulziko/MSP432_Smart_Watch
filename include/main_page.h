/*
 * main_page.h
 *
 *  Created on: 18 dic 2025
 *      Author: Lenovo
 */

#ifndef MAIN_PAGE_H_
#define MAIN_PAGE_H_


#define STICK_DEAD_ZONE 1000
#define STICK_HORIZONTAL_THRESHOLD 1000
#define ADC_MAX_VALUE (1 << 14)
#define MAIN_BACKGROUND_COLOR_BLUE  0x00363E69

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"

#define TEXT_OFFSET 10
#define TEXT_SPACING 25


void main_page_task(Graphics_Context *pContext);


void main_page_adc(uint64_t status, uint16_t* conversionValues);

void main_page_button();

void main_page_exit();

#endif /* MAIN_PAGE_H_ */
