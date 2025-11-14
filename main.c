#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>

/* Graphic library context */
Graphics_Context g_sContext;

/* Variable for storing lux value returned from OPT3001 */
float lux;

void _lightSensorInit()
{
    /* Initialize I2C communication */
    Init_I2C_GPIO();
    I2C_init();

    /* Initialize OPT3001 digital ambient light sensor */
    OPT3001_init();

    __delay_cycles(100000);

}

void _graphicsInit()
{
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);

}

void _hwInit()
{
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    _graphicsInit();
    _lightSensorInit();
}

/*
 * Main function
 */

void _graphicsDisplay_set_red()
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
}

void _graphicsDisplay_set_blue()
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);
}

int main(void)
{

    _hwInit();

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "Light Sensor:",
    AUTO_STRING_LENGTH,
                                64, 30,
                                OPAQUE_TEXT);

    while (1)
    {

        /* Obtain lux value from OPT3001 */
        lux = OPT3001_getLux();
        if (lux <5000){
            _graphicsDisplay_set_blue();
        }
        else {
            _graphicsDisplay_set_red();
        }
        char string[20];
//
        sprintf(string, "%f", lux);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) string, 6, 48, 70,
        OPAQUE_TEXT);
//
        sprintf(string, "lux");
        Graphics_drawStringCentered(&g_sContext, (int8_t *) string, 3, 86, 70,
        OPAQUE_TEXT);
        // drawing circle
        Graphics_drawCircle(&g_sContext, 33, 64, 25);
        Graphics_fillCircle(&g_sContext, 95, 64, 25);

    }
}

