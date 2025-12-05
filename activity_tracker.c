#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"

Graphics_Context g_sContext;

static uint16_t resultsBuffer[3];

static uint32_t stepCount = 0;
static double distance_cm = 0.0;
static bool stepDetected = false;

#define STEP_LENGTH_CM 7.0            /* step length in cm */
#define GOAL_STEPS 10000               /* goal steps per day */
#define GOAL_DISTANCE_CM (STEP_LENGTH_CM * (double)GOAL_STEPS) /* 700000 cm = 7 km */

#define REFRACTORY_MS 300
#define WINDOW_SIZE 50
#define SCALE_FACTOR 1.2

static double magBuffer[WINDOW_SIZE];
static uint32_t bufferIndex = 0;
static double rmsValue = 0.0;
static uint32_t lastStepTime = 0;

void drawStepData(double magnitude, double thresholdHigh);
void drawCurrentTime(void);

uint32_t getTimeMs(void)
{
    static uint32_t fakeTime = 0;
    return fakeTime += 40; // ~25 Hz sampling tick (keeps algorithm timing consistent)
}

void RTC_init(void)
{
    RTC_C_Calendar time = {
        .seconds    = 0,
        .minutes    = 0,
        .hours      = 0,
        .dayOfWeek  = 1,
        .dayOfmonth = 1,
        .month      = 1,
        .year       = 2025
    };

    RTC_C_initCalendar(&time, RTC_C_FORMAT_BINARY);

    RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MIDNIGHT);
    RTC_C_enableInterrupt(RTC_C_TIME_EVENT_INTERRUPT);

    RTC_C_startClock();
}

void RTC_setDateTime(uint8_t hour, uint8_t min, uint8_t sec,
                     uint8_t day, uint8_t month, uint16_t year)
{
    RTC_C_holdClock();

    RTC_C_Calendar newTime = {
        .seconds    = sec,
        .minutes    = min,
        .hours      = hour,
        .dayOfWeek  = 1,      // dummy; not used by clock
        .dayOfmonth = day,
        .month      = month,
        .year       = year
    };

    RTC_C_initCalendar(&newTime, RTC_C_FORMAT_BINARY);
    RTC_C_startClock();
}


void drawCurrentTime(void)
{
    RTC_C_Calendar t = RTC_C_getCalendarTime();
    char buffer[32];

    sprintf(buffer, "%02d:%02d:%02d %02d/%02d/%04d",
            t.hours, t.minutes, t.seconds,
            t.dayOfmonth, t.month, t.year);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)buffer,
                                AUTO_STRING_LENGTH,
                                64, 118, OPAQUE_TEXT);
}

void RTC_C_IRQHandler(void)
{
    uint32_t status = RTC_C_getEnabledInterruptStatus();
    RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {
        stepCount = 0;
        distance_cm = 0;

        // Clear filter buffer
        int i;
        for (i = 0; i < WINDOW_SIZE; i++)
            magBuffer[i] = 0;

        bufferIndex = 0;
        stepDetected = false;

        // Redraw display
        Graphics_clearDisplay(&g_sContext);
        drawStepData(0, 0);
    }
}


/* ===== Graphics init and helpers ===== */
void _graphicsInit(void)
{
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);
}

void drawProgressBar(double percent)
{
    Graphics_Rectangle outline = {20, 20, 108, 35};
    Graphics_drawRectangle(&g_sContext, &outline);

    /* limit percent between 0..100 */
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;

    int fillWidth = (int)(percent * (108 - 20) / 100.0);
    Graphics_Rectangle fill = {20, 20, 20 + fillWidth, 35};
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
    Graphics_fillRectangle(&g_sContext, &fill);

    char percentStr[12];
    sprintf(percentStr, "%.1f%%", percent);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext, (int8_t *)percentStr,
                                AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);
}

/* ===== Draw step data and progress ===== */
void drawStepData(double magnitude, double thresholdHigh)
{
    char buffer[32];
    Graphics_clearDisplay(&g_sContext);

    double progress = 0.0;
    if (GOAL_DISTANCE_CM > 0.0)
        progress = (distance_cm / GOAL_DISTANCE_CM) * 100.0;

    drawProgressBar(progress);

    sprintf(buffer, "%lu", (unsigned long)stepCount);
    Graphics_drawStringCentered(&g_sContext, (int8_t *)buffer,
                                AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);

    Graphics_drawStringCentered(&g_sContext, (int8_t *)"steps",
                                AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);

    char debugStr[32];
    sprintf(debugStr, "Mag:%.0f Th:%.0f", magnitude, thresholdHigh);
    Graphics_drawStringCentered(&g_sContext, (int8_t *)debugStr,
                                AUTO_STRING_LENGTH, 64, 104, OPAQUE_TEXT);

    drawCurrentTime();
}

void _accelSensorInit(void)
{
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4,
                                               GPIO_PIN0 | GPIO_PIN2,
                                               GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6,
                                               GPIO_PIN1,
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

    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}

void ADC14_IRQHandler(void)
{
    uint64_t status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if (status & ADC_INT2)
    {
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0); // accel X
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1); // accel Y
        resultsBuffer[2] = ADC14_getResult(ADC_MEM2); // accel Z

        /* compute magnitude */
        double magnitude = sqrt(
            (double)resultsBuffer[0] * (double)resultsBuffer[0] +
            (double)resultsBuffer[1] * (double)resultsBuffer[1] +
            (double)resultsBuffer[2] * (double)resultsBuffer[2]);

        /* update circular buffer */
        magBuffer[bufferIndex] = magnitude;
        bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;

        /* compute RMS */
        double sumSq = 0.0;
        int i;
        for (i = 0; i < WINDOW_SIZE; ++i) sumSq += magBuffer[i] * magBuffer[i];
        rmsValue = sqrt(sumSq / WINDOW_SIZE);

        double thresholdHigh = SCALE_FACTOR * rmsValue;
        double thresholdLow  = 0.95 * rmsValue;

        uint32_t now = getTimeMs();

        /* step detection with refractory */
        if (!stepDetected && magnitude > thresholdHigh &&
            (now - lastStepTime) > REFRACTORY_MS)
        {
            stepDetected = false;
            stepCount++;
            distance_cm = (double)stepCount * STEP_LENGTH_CM;
            lastStepTime = now;
        }
        else if (stepDetected && magnitude < thresholdLow)
        {
            stepDetected = false;
        }

        drawStepData(magnitude, thresholdHigh);
    }
}

void _hwInit(void)
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

    _graphicsInit();
    _accelSensorInit();

    // 1) Set the date/time BEFORE enabling RTC events
    RTC_C_holdClock();

    RTC_C_Calendar t = {
        .seconds    = 50,
        .minutes    = 59,
        .hours      = 23,
        .dayOfWeek  = 2,
        .dayOfmonth = 27,
        .month      = 1,
        .year       = 2025
    };

    RTC_C_initCalendar(&t, RTC_C_FORMAT_BINARY);

    // 2) Now configure MIDNIGHT interrupt
    RTC_C_enableInterrupt(RTC_C_TIME_EVENT_INTERRUPT);
    RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MIDNIGHT);

    // 3) Start RTC AFTER all configs are done
    RTC_C_startClock();

    // Set initial steps (DO THIS BEFORE ENABLE MASTER)
    stepCount = 3600;
    distance_cm = stepCount * STEP_LENGTH_CM;

    Interrupt_enableInterrupt(INT_RTC_C);
    Interrupt_enableMaster();
}


int main(void)
{
    _hwInit();

    drawStepData(0.0, 0.0);

    while (1)
    {
        PCM_gotoLPM0();
    }
}
