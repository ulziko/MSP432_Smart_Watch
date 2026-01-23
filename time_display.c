#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "system_time.h"
#include "time_display.h"


#define  MAIN_BACKGROUND_COLOR_BLUE  0x00363E69
int timeUpdateFlag=1;
int init_display=true;

void DrawTime(Graphics_Context *pContext)
{
    sprintf(ClockTime.time_string, "%02d:%02d:%02d", ClockTime.hour, ClockTime.minute, ClockTime.second);
    GrContextFontSet(pContext, &g_sFontCmsc34);
    Graphics_drawStringCentered(pContext, (int8_t*)ClockTime.time_string, -1, 64, 60, OPAQUE_TEXT);
}

void DrawDate(Graphics_Context *pContext)
{
    sprintf(ClockDate.date_string, "%02d/%02d/%04d", ClockDate.day, ClockDate.month, ClockDate.year);
    GrContextFontSet(pContext, &g_sFontCmsc12);
    Graphics_drawStringCentered(pContext, (int8_t*)ClockDate.date_string, -1, 64, 110, OPAQUE_TEXT);
}

// called when task switched to the time_display
void time_display(Graphics_Context *pContext){
    if(init_display){
        Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_WHITE);
        Graphics_setBackgroundColor(pContext, MAIN_BACKGROUND_COLOR_BLUE);
        Graphics_clearDisplay(pContext);
        init_display=false;
    }
    // Check if timer interrupt set the update flag
    if (timeUpdateFlag) {
        timeUpdateFlag = 0;  // Clear flag
        // Redraw
        DrawTime(pContext);
        DrawDate(pContext);
    }

    //wake on timer interrupt
    PCM_gotoLPM0();
}

// Called from timer interrupt in main.c
void time_display_ta1_handler(void) {
    timeUpdateFlag = 1;
}
