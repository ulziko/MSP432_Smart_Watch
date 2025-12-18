#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "tasks.h"
#include "system_time.h"


#define STICK_DEAD_ZONE 1000
#define STICK_HORIZONTAL_THRESHOLD 1000
#define ADC_MAX_VALUE (1 << 14)



#define WRONG_WD_PWD 0x00FF;


#define TOTAL_ADC_VALUES 3

uint16_t conversion_values[TOTAL_ADC_VALUES];

TimeStruct ClockTime = {0, 0, 0, ""};
DateStruct ClockDate = {0, 0, 0, ""};

void reset_device() {

    //The watchdog timer will reset the device if a wrong password is inserted.
    WDT_A->CTL &= WRONG_WD_PWD;

    //Loop until reset complete.
    while (1);
}


void initGraphics(Graphics_Context *g_sContext) {

    /* Halting WDT and disabling master interrupts */
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_RED);
    Graphics_setBackgroundColor(g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(g_sContext);
    /* Reenabe the interrupt so that we get interrupt again*/
    MAP_Interrupt_enableMaster();
}


initADC() {

    /* Configures Pin 6.0 and 4.4 as ADC input */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8) */
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
         * with internal 2.5v reference */
    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
    MAP_ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     *  is complete and enabling conversions */
    MAP_ADC14_enableInterrupt(ADC_INT1);

    /* Enabling Interrupts */
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();
}


// this timer initializes the timer_A_Clocksource.
void initTimer() {
    // configuring Timer A as an UpMode
    Timer_A_UpModeConfig upConfig = {
        TIMER_A_CLOCKSOURCE_ACLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        32767,  // 1 second (32768 - 1)
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
        TIMER_A_DO_CLEAR
    };
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);
    MAP_Timer_A_clearTimer(TIMER_A0_BASE);
    MAP_Interrupt_enableInterrupt(INT_TA0_0);
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
    syncTime();
}


tasks_t current_task = TIME_DISPLAY;



/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer


	/* Graphic library context */
	Graphics_Context g_sContext;

	initGraphics(&g_sContext);

	//Initialize timer
	initTimer();

	//initADC();


	while (1) {
	    if (current_task < TASK_COUNT)
	        handlers[current_task].main_handler(&g_sContext);
	    else
	        //Something very wrong has happened. Reset everything.
	        reset_device();
	}
}






/* This interrupt is fired whenever a conversion is completed and placed in ADC_MEM1.
 * The execution is then passed to the proper task using the conversion results as parameters.
 */
void ADC14_IRQHandler(void)
{
    uint64_t status;

    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);


    uint32_t i;
    for (i = 0; i < DIM(conversion_values); i ++)
        conversion_values[i] = 1 << i;

    handlers[current_task].adc_handler(status, conversion_values);
    //adc_task_handlers[current_task](status, conversion_values);
}



// Timer A0 interrupt - fires every 1 second
void TA0_0_IRQHandler(void) {
    MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,
                                            TIMER_A_CAPTURECOMPARE_REGISTER_0);

    // Updates system time.
    updateTime();

    handlers[current_task].adc_handler(status, conversion_values);
}




void detect_stick_movement()
{
    uint64_t status;

    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);




    static uint32_t stick_released = 0;



    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {

        /* ADC results buffer */
        uint16_t resultsBuffer[2];

        //* Store ADC14 conversion results *
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);



        //Determine if the stick is released (x and y are both inside the deadzone)
        stick_released |= ADC_MAX_VALUE / 2 - STICK_DEAD_ZONE < resultsBuffer[0] && resultsBuffer[0] < ADC_MAX_VALUE / 2 + STICK_DEAD_ZONE &&
                ADC_MAX_VALUE / 2 - STICK_DEAD_ZONE < resultsBuffer[1] && resultsBuffer[1] < ADC_MAX_VALUE / 2 + STICK_DEAD_ZONE;



        //If they went in the deadzone already...
        if (stick_released) {

            //Check if they are tilted
            if (resultsBuffer[0] < STICK_HORIZONTAL_THRESHOLD) {

                if (current_task >= TASK_COUNT)
                    current_task = (tasks_t) 0;
                else
                    current_task ++;

                stick_released = 0;
            }
            if (resultsBuffer[0] > ADC_MAX_VALUE - STICK_HORIZONTAL_THRESHOLD) {

                if (current_task == 0)
                    current_task = (tasks_t) (TASK_COUNT - 1);
                else
                    current_task --;

                stick_released = 0;
            }
        }
    }//*/
}

