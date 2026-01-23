/*
 * main_page.c
 *
 *  Created on: 18 dic 2025
 *      Author: Lenovo
 */


#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "tasks.h"


Graphics_Context *pContext = 0;


uint32_t chosen_task = TIME_DISPLAY;


void stick_tilted_up();

void stick_tilted_down();


void draw_menu_text();




void main_page_task(Graphics_Context *_pContext) {
    //Draw the menu if this is the first time passing from here.
    if (pContext == 0) {
        pContext = _pContext;
        draw_menu_text();
    }
}


void main_page_adc(uint64_t status, uint16_t* resultsBuffer) {

    //If the context wasn't yet initialized, return and wait for the thread to initialize it.
    if (pContext == 0)
        return;


    static uint32_t stick_released = 0;



    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {
        //Determine if the stick is released (x and y are both inside the deadzone)
        stick_released |= ADC_MAX_VALUE / 2 - STICK_DEAD_ZONE < resultsBuffer[0] && resultsBuffer[0] < ADC_MAX_VALUE / 2 + STICK_DEAD_ZONE &&
                ADC_MAX_VALUE / 2 - STICK_DEAD_ZONE < resultsBuffer[1] && resultsBuffer[1] < ADC_MAX_VALUE / 2 + STICK_DEAD_ZONE;



        //If they went in the deadzone already...
        if (stick_released) {

            //Check if they are tilted
            if (resultsBuffer[1] < STICK_HORIZONTAL_THRESHOLD) {

                stick_tilted_down();
                draw_menu_text();

                stick_released = 0;
            }
            if (resultsBuffer[1] > ADC_MAX_VALUE - STICK_HORIZONTAL_THRESHOLD) {

                stick_tilted_up();
                draw_menu_text();

                stick_released = 0;
            }
        }
    }
}

void stick_tilted_up() {
    if (handlers[chosen_task].task_enum - 1 != MAIN_PAGE)
        chosen_task --;
}

void stick_tilted_down() {
    if (handlers[chosen_task].task_enum + 1 != TASK_COUNT)
        chosen_task ++;
}





void set_colored_text() {
    Graphics_setForegroundColor(pContext, MAIN_BACKGROUND_COLOR_BLUE);
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_WHITE);
}
void set_colored_foreground() {
    Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(pContext, MAIN_BACKGROUND_COLOR_BLUE);
}


void draw_menu_text() {

    Graphics_clearDisplay(pContext);

    int i;
    for (i = 1; i < TASK_COUNT; i ++) {
        if (i == chosen_task)
            set_colored_foreground();
        else
            set_colored_text();


        Graphics_drawStringCentered(pContext,
                                        handlers[i].task_name,
                                        AUTO_STRING_LENGTH,
                                        64,
                                        i * 25 + 25,
                                        OPAQUE_TEXT);
    }

    set_colored_text();
}


void main_page_button() {
    current_task = handlers[chosen_task].task_enum;
    pContext = 0;
}

