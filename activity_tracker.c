#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <math.h>

Graphics_Context* g_sContext;

static uint16_t resultsBuffer[3];

static uint32_t stepCount = 0;
static float distance_cm = 0;
static bool stepDetected = false;

#define STEP_LENGTH_CM 2.0     // step length
#define GOAL_DISTANCE_CM 100.0 // goal dist
#define REFRACTORY_MS 100

#define WINDOW_SIZE 20
#define SCALE_FACTOR 1.1

static double magBuffer[WINDOW_SIZE];
static uint32_t bufferIndex = 0;
static double rmsValue = 0;
static uint32_t lastStepTime = 0;

uint32_t getTimeMs(void)
{
    static uint32_t fakeTime = 0;
    return fakeTime += 40; // assume 25 Hz sampling
}



//Already initialized in main.c
/*

void _graphicsInit()
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_NAVY);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
}*/

void drawProgressBar(float percent)
{
    Graphics_Rectangle outline = {20, 20, 108, 35};
    Graphics_drawRectangle(g_sContext, &outline);

    int fillWidth = (int)(percent * (108 - 20) / 100.0);
    Graphics_Rectangle fill = {20, 20, 20 + fillWidth, 35};
    Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_GREEN);
    Graphics_fillRectangle(g_sContext, &fill);

    char percentStr[10];
    sprintf(percentStr, "%.1f%%", percent);
    Graphics_setForegroundColor(g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(g_sContext, (int8_t *)percentStr,
                                AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);
}

void drawStepData(double magnitude, double thresholdHigh)
{
    char buffer[20];
    Graphics_clearDisplay(g_sContext);

    float progress = (distance_cm / GOAL_DISTANCE_CM) * 100.0;
    drawProgressBar(progress);

    sprintf(buffer, "%lu", stepCount);
    Graphics_drawStringCentered(g_sContext, (int8_t *)buffer,
                                AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);

    Graphics_drawStringCentered(g_sContext, (int8_t *)"steps",
                                AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);

    char debugStr[20];
    sprintf(debugStr, "Mag:%.0f Th:%.0f", magnitude, thresholdHigh);
    Graphics_drawStringCentered(g_sContext, (int8_t *)debugStr,
                                AUTO_STRING_LENGTH, 64, 110, OPAQUE_TEXT);
}

void _accelSensorInit()
{
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4,
                                               GPIO_PIN0 | GPIO_PIN2,
                                               GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN1,
                                               GPIO_TERTIARY_MODULE_FUNCTION);

    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM2, true);
    ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A14, ADC_NONDIFFERENTIAL_INPUTS);
    ADC14_configureConversionMemory(ADC_MEM1, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A13, ADC_NONDIFFERENTIAL_INPUTS);
    ADC14_configureConversionMemory(ADC_MEM2, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A11, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_enableInterrupt(ADC_INT2);
    Interrupt_enableInterrupt(INT_ADC14);
    Interrupt_enableMaster();

    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}

void _hwInit()
{
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    PCM_setCoreVoltageLevel(PCM_VCORE1);
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    //_graphicsInit();
    _accelSensorInit();
}

//void ADC14_IRQHandler(void)
void activity_tracker_adc(uint64_t status, uint16_t* conversion_values)
{
    //uint64_t status = ADC14_getEnabledInterruptStatus();
    //ADC14_clearInterruptFlag(status);

    if (status & ADC_INT2)
    {
        /*resultsBuffer[0] = ADC14_getResult(ADC_MEM0); // X
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1); // Y
        resultsBuffer[2] = ADC14_getResult(ADC_MEM2); // Z*/

        resultsBuffer[0] = conversion_values[0];
        resultsBuffer[1] = conversion_values[1];
        resultsBuffer[2] = conversion_values[2];

        // calculate magnitude
        double magnitude = sqrt(
            (double)resultsBuffer[0]*resultsBuffer[0] +
            (double)resultsBuffer[1]*resultsBuffer[1] +
            (double)resultsBuffer[2]*resultsBuffer[2]);

        // update buffer
        magBuffer[bufferIndex] = magnitude;
        bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;

        double sumSq = 0;
        int i;
        for (i = 0; i < WINDOW_SIZE; i++) {
            sumSq += magBuffer[i] * magBuffer[i];
        }
        rmsValue = sqrt(sumSq / WINDOW_SIZE);

        double thresholdHigh = SCALE_FACTOR * rmsValue;
        double thresholdLow  = 0.8 * rmsValue;

        uint32_t now = getTimeMs();

        if (!stepDetected && magnitude > thresholdHigh &&
            (now - lastStepTime) > REFRACTORY_MS)
        {
            stepDetected = true;
            stepCount++;
            distance_cm = stepCount * STEP_LENGTH_CM;
            drawStepData(magnitude, thresholdHigh);
            lastStepTime = now;
        }
        else if (stepDetected && magnitude < thresholdLow)
        {
            stepDetected = false;
        }

        drawStepData(magnitude, thresholdHigh);
    }
}

void activity_tracker(Graphics_Context *pContext)
{
    _hwInit();

    g_sContext = pContext;

    drawStepData(0, 0);

    while (1)
    {
        PCM_gotoLPM0();
    }
}
