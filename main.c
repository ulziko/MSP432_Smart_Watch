#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <time.h>

/* Graphic library context */
Graphics_Context g_sContext;

typedef struct {
    int hour;
    int minute;
    int second;
    char time_string[20];
} TimeStruct;

typedef struct{
    int day;
    int month;
    int year;
    char date_string[20];
} DateStruct;

TimeStruct ClockTime;
DateStruct ClockDate;



/* Timer_A configuration for 1 second interrupt (ACLK = 32768 Hz) */
const Timer_A_UpModeConfig upConfig = {
    TIMER_A_CLOCKSOURCE_ACLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_1,
    32767,                    // 1 second
    TIMER_A_TAIE_INTERRUPT_DISABLE,
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
    TIMER_A_DO_CLEAR
};

void _graphicsInit()
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE_SMOKE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_SLATE_BLUE);
    GrContextFontSet(&g_sContext, &g_sFontCmsc24);
    Graphics_clearDisplay(&g_sContext);
}

void _hwInit()
{
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    PCM_setCoreVoltageLevel(PCM_VCORE1);
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);
    Interrupt_enableInterrupt(INT_TA0_0);
    Interrupt_enableMaster();

    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

void syncTime()
{
    time_t current_time;
    current_time = time(NULL);
    struct tm *t = localtime(&current_time);
    time ( &current_time );
    ClockTime.hour = t->tm_hour;
    ClockTime.minute = t->tm_min;
    ClockTime.second = t->tm_sec;

    ClockDate.day   = t->tm_mday;
    ClockDate.month = t->tm_mon + 1;
    ClockDate.year  = t->tm_year + 1900;
}

void DrawTime()
{
    sprintf(ClockTime.time_string, "%02d:%02d:%02d", ClockTime.hour, ClockTime.minute, ClockTime.second);
    GrContextFontSet(&g_sContext, &g_sFontCmsc34);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)ClockTime.time_string, -1, 64, 60, OPAQUE_TEXT);
}

void DrawDate()
{
    sprintf(ClockDate.date_string, "%02d/%02d/%04d", ClockDate.day, ClockDate.month, ClockDate.year);
    GrContextFontSet(&g_sContext, &g_sFontCmsc12);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)ClockDate.date_string, -1, 64, 110, OPAQUE_TEXT);
}

int main(void)
{
    _graphicsInit();
    syncTime();

    _hwInit();
    DrawTime();
    DrawDate();

    while (1)
        PCM_gotoLPM0();
}

void TA0_0_IRQHandler(void)
{
    // this section is neccesary
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
    // second timer
    ClockTime.second++;
    if(ClockTime.second >= 60){
        ClockTime.second = 0;
        ClockTime.minute++;
    }
    if(ClockTime.minute >= 60){
        ClockTime.minute = 0;
        ClockTime.hour++;
    }
    if(ClockTime.hour >= 24){
        ClockTime.hour = 0;
        syncTime();
    }

    DrawTime();
    DrawDate();
}