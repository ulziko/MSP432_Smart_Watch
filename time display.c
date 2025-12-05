#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"

#include <stdio.h>

Graphics_Context* g_sContext;



/* This is not needed anymore: the display is initialized in main.c and the context is passed as parameter
void LCD_init(void)
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_NAVY);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Time",
                                AUTO_STRING_LENGTH,
                                64, 20, OPAQUE_TEXT);
}*/

void LCD_drawTime(void)
{
    RTC_C_Calendar t = RTC_C_getCalendarTime();
    char buffer[16];
    sprintf(buffer, "%02d:%02d:%02d", t.hours, t.minutes, t.seconds);

    Graphics_Rectangle box = {20, 45, 108, 65};
    Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_NAVY);
    Graphics_fillRectangle(g_sContext, &box);

    Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(g_sContext,
                                (int8_t *)buffer,
                                AUTO_STRING_LENGTH,
                                64, 55, OPAQUE_TEXT);
}

/* ===== RTC INIT ===== */
void RTC_init(void)
{
    RTC_C_Calendar time = {
        .seconds    = 0,
        .minutes    = 30,
        .hours      = 14,  
        .dayOfWeek  = 4,
        .dayOfmonth = 23,
        .month      = 11,
        .year       = 2025
    };

    RTC_C_initCalendar(&time, RTC_C_FORMAT_BINARY);

    /* Enable READ READY interrupt (1 Hz) */
    RTC_C_enableInterrupt(RTC_C_CLOCK_READ_READY_INTERRUPT);

    RTC_C_startClock();
}

/* ===== RTC ISR — Fires every second ===== */
void RTC_C_IRQHandler(void)
{
    uint32_t status = RTC_C_getEnabledInterruptStatus();
    RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)
    {
        LCD_drawTime();   // Update just the time area
    }
}

void time_display(Graphics_Context *pContext)
{
    //Get the context
    g_sContext = pContext;


    //Already done previosly
    //WDT_A_holdTimer();

    /* Set system clock to 3 MHz */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_3);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    //LCD_init();
    RTC_init();

    Interrupt_enableInterrupt(INT_RTC_C);
    Interrupt_enableMaster();   //I think this could me moved to main.c

    while (1)
    {
        PCM_gotoLPM0();    // sleep until RTC interrupt
    }
}
