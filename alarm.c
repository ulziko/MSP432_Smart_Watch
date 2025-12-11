#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"


/* BoosterPack buttons */
#define STOP_BUTTON_PORT   GPIO_PORT_P5    // S1
#define STOP_BUTTON_PIN    GPIO_PIN1
#define SNOOZE_BUTTON_PORT GPIO_PORT_P3    // S2
#define SNOOZE_BUTTON_PIN  GPIO_PIN5

/* Buzzer on P2.7 using Timer_A0.4 PWM */
#define BUZZER_PORT GPIO_PORT_P2
#define BUZZER_PIN  GPIO_PIN7

/* Snooze duration */
const int g_snoozeMinutes = 1;

Timer_A_UpModeConfig upConfigBuzzer = {
    TIMER_A_CLOCKSOURCE_SMCLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_1,
    3000,                                   // 3 MHz / 3000 = 1 kHz
    TIMER_A_TAIE_INTERRUPT_DISABLE,
    TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
    TIMER_A_DO_CLEAR
};

Timer_A_CompareModeConfig compareConfig_PWM = {
    TIMER_A_CAPTURECOMPARE_REGISTER_4,      // TA0.4 → P2.7
    TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
    TIMER_A_OUTPUTMODE_RESET_SET,
    0                                       // start silent
};

/* Timer_A1: 1-second tick for software clock (ACLK ~ 32768 Hz) */
Timer_A_UpModeConfig upConfigSecond = {
    TIMER_A_CLOCKSOURCE_ACLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_1,
    32767,                                  // ≈ 1 second at ~32768 Hz
    TIMER_A_TAIE_INTERRUPT_DISABLE,
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
    TIMER_A_DO_CLEAR
};

typedef struct {
    int hour;
    int minute;
    int second;
} TimeStruct;

typedef struct {
    int day;
    int month;
    int year;
} DateStruct;

TimeStruct ClockTime;
DateStruct ClockDate;

Graphics_Context g_sContext;

/* Alarm time being edited by joystick */
int g_hour   = 0;
int g_minute = 0;
/* 0 = hour column, 1 = minute column */
int g_selectedField = 0;

/* Confirmed alarm time (after pressing S1 when idle) */
int g_alarmHour   = 0;
int g_alarmMinute = 0;
uint8_t g_alarmSet     = 0;  // 1 when alarm is set
uint8_t g_alarmRinging = 0;  // 1 when alarm is currently ringing

/* Joystick ADC results (A15 = X, A9 = Y) */
volatile uint16_t g_joyX = 8192;
volatile uint16_t g_joyY = 8192;

/* Previous state of buttons for edge detect */
uint8_t g_prevStopButton   = 1;
uint8_t g_prevSnoozeButton = 1;

void LCD_init(void);
void drawCurrentTime(void);
void drawAlarmScreen(void);

void joystickInit(void);
int16_t readJoystickX(void);
int16_t readJoystickY(void);
void updateFromJoystick(void);

void buttonsInit(void);
void checkButtons(void);

void _buzzerInit(void);
void buzzerOn(void);
void buzzerOff(void);

void clockInit(void);
void syncTime(void);

void ADC14_IRQHandler(void);
void TA1_0_IRQHandler(void);

void LCD_init(void)
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

/* Show current time at top: HH:MM:SS */
void drawCurrentTime(void)
{
    char buf[16];
    Graphics_Rectangle timeArea;

    /* Clear top strip */
    timeArea.xMin = 0;
    timeArea.yMin = 0;
    timeArea.xMax = 127;
    timeArea.yMax = 16;
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_fillRectangle(&g_sContext, &timeArea);

    sprintf(buf, "%02d:%02d:%02d",
            ClockTime.hour, ClockTime.minute, ClockTime.second);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                64, 8,
                                OPAQUE_TEXT);
}

/* Draw the whole alarm UI: title, picker, and status text */
void drawAlarmScreen(void)
{
    Graphics_Rectangle band, msgArea, statusArea;
    char buf[32];

    /* Title under the time */
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"Set Alarm",
                                AUTO_STRING_LENGTH,
                                64, 20, OPAQUE_TEXT);

    /* Clear message area (y ~ 24-52) */
    msgArea.xMin = 0;  msgArea.yMin = 24;
    msgArea.xMax = 127; msgArea.yMax = 52;
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_fillRectangle(&g_sContext, &msgArea);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    if (g_alarmRinging)
    {
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"ALARM RINGING!",
                                    AUTO_STRING_LENGTH,
                                    64, 30, OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"S1=Stop  S2=Snooze",
                                    AUTO_STRING_LENGTH,
                                    64, 42, OPAQUE_TEXT);
    }
    else
    {
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"Use joystick to set",
                                    AUTO_STRING_LENGTH,
                                    64, 30, OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"S1=Set/Stop  S2=--",
                                    AUTO_STRING_LENGTH,
                                    64, 42, OPAQUE_TEXT);
    }

    /* Middle selection band */
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_GRAY);
    band.xMin = 4;
    band.yMin = 64;
    band.xMax = 123;
    band.yMax = 80;
    Graphics_fillRectangle(&g_sContext, &band);

    /* Vertical separator between hour and minute */
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GRAY);
    Graphics_drawLineV(&g_sContext, 64, 52, 92);

    /* Coordinates for the "wheel" */
    int centerY = 72;
    int offsetY = 16;
    int hourX   = 40;
    int minX    = 96;

    int prevHour = (g_hour + 23) % 24;
    int nextHour = (g_hour + 1)  % 24;
    int prevMin  = (g_minute + 59) % 60;
    int nextMin  = (g_minute + 1)  % 60;

    /* Small faded font for top/bottom rows */
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GRAY);

    /* Previous hour/minute (top row) */
    sprintf(buf, "%02d", prevHour);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY - offsetY, OPAQUE_TEXT);

    sprintf(buf, "%02d", prevMin);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY - offsetY, OPAQUE_TEXT);

    /* Next hour/minute (bottom row) */
    sprintf(buf, "%02d", nextHour);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY + offsetY, OPAQUE_TEXT);

    sprintf(buf, "%02d", nextMin);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY + offsetY, OPAQUE_TEXT);

    /* Selected hour in middle row */
    if (g_selectedField == 0)
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    else
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GRAY);

    sprintf(buf, "%02d", g_hour);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY, OPAQUE_TEXT);

    /* Selected minute in middle row */
    if (g_selectedField == 1)
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    else
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GRAY);

    sprintf(buf, "%02d", g_minute);
    Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY, OPAQUE_TEXT);

    /* Bottom status line area */
    statusArea.xMin = 0;   statusArea.yMin = 100;
    statusArea.xMax = 127; statusArea.yMax = 127;

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_fillRectangle(&g_sContext, &statusArea);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    if (g_alarmSet)
    {
        if (g_alarmRinging)
        {
            Graphics_drawStringCentered(&g_sContext,
                                        (int8_t*)"ALARM NOW!",
                                        AUTO_STRING_LENGTH,
                                        64, 110, OPAQUE_TEXT);
        }
        else
        {
            sprintf(buf, "Alarm set: %02d:%02d", g_alarmHour, g_alarmMinute);
            Graphics_drawStringCentered(&g_sContext, (int8_t*)buf,
                                        AUTO_STRING_LENGTH,
                                        64, 110, OPAQUE_TEXT);
        }
    }
    else
    {
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t*)"No alarm set",
                                    AUTO_STRING_LENGTH,
                                    64, 110, OPAQUE_TEXT);
    }
}


void updateFromJoystick(void)
{
    if (g_alarmRinging)
        return;  // ignore joystick while ringing

    const int16_t LOW_THRESH  = 5000;
    const int16_t HIGH_THRESH = 12000;
    static uint8_t yReleased = 1;

    int16_t x = readJoystickX();
    int16_t y = readJoystickY();

    if (x < LOW_THRESH)
        g_selectedField = 0;           // hour
    else if (x > HIGH_THRESH)
        g_selectedField = 1;           // minute

    if (y < LOW_THRESH && yReleased) {         // pushed UP
        if (g_selectedField == 0)
            g_hour = (g_hour + 23) % 24;       // hour--
        else
            g_minute = (g_minute + 59) % 60;   // minute--
        yReleased = 0;
    }
    else if (y > HIGH_THRESH && yReleased) {   // pushed DOWN
        if (g_selectedField == 0)
            g_hour = (g_hour + 1) % 24;        // hour++
        else
            g_minute = (g_minute + 1) % 60;    // minute++
        yReleased = 0;
    }
    else if (y >= LOW_THRESH && y <= HIGH_THRESH) {
        yReleased = 1;
    }
}

/* Joystick ADC init (A15 = X on P6.0, A9 = Y on P4.4) */
void joystickInit(void)
{
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0,
                                               GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4,
                                               GPIO_TERTIARY_MODULE_FUNCTION);

    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64,
                     ADC_DIVIDER_8, 0);

    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);

    ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_enableInterrupt(ADC_INT1);
    Interrupt_enableInterrupt(INT_ADC14);

    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}

void ADC14_IRQHandler(void)
{
    uint64_t status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if (status & ADC_INT1)
    {
        g_joyX = ADC14_getResult(ADC_MEM0);
        g_joyY = ADC14_getResult(ADC_MEM1);
    }
}

int16_t readJoystickX(void)
{
    return (int16_t)g_joyX;
}

int16_t readJoystickY(void)
{
    return (int16_t)g_joyY;
}


void buttonsInit(void)
{
    GPIO_setAsInputPinWithPullUpResistor(STOP_BUTTON_PORT,   STOP_BUTTON_PIN);
    GPIO_setAsInputPinWithPullUpResistor(SNOOZE_BUTTON_PORT, SNOOZE_BUTTON_PIN);

    g_prevStopButton   = GPIO_getInputPinValue(STOP_BUTTON_PORT,   STOP_BUTTON_PIN);
    g_prevSnoozeButton = GPIO_getInputPinValue(SNOOZE_BUTTON_PORT, SNOOZE_BUTTON_PIN);
}

void checkButtons(void)
{
    uint8_t currStop   = GPIO_getInputPinValue(STOP_BUTTON_PORT,   STOP_BUTTON_PIN);
    uint8_t currSnooze = GPIO_getInputPinValue(SNOOZE_BUTTON_PORT, SNOOZE_BUTTON_PIN);

    /* S1: stop (if ringing) or set/cancel alarm (if idle) */
    if (currStop == 0 && g_prevStopButton == 1)
    {
        if (g_alarmRinging)
        {
            /* STOP alarm completely */
            g_alarmRinging = 0;
            g_alarmSet     = 0;    // disable alarm
            buzzerOff();
        }
        else
        {
            /* SET / UPDATE alarm from picker */
            g_alarmSet   = 1;
            g_alarmHour   = g_hour;
            g_alarmMinute = g_minute;
        }
    }

    /* S2: snooze only when ringing */
    if (currSnooze == 0 && g_prevSnoozeButton == 1)
    {
        if (g_alarmRinging)
        {
            /* SNOOZE: stop buzzer, schedule alarm after g_snoozeMinutes */
            g_alarmRinging = 0;
            buzzerOff();

            int totalMinutes = ClockTime.hour * 60 + ClockTime.minute + g_snoozeMinutes;
            g_alarmHour   = (totalMinutes / 60) % 24;
            g_alarmMinute = totalMinutes % 60;
            g_alarmSet    = 1;
        }
        /* else: ignore S2 when not ringing */
    }

    g_prevStopButton   = currStop;
    g_prevSnoozeButton = currSnooze;
}

/* Buzzer (Timer_A0 PWM) */

void _buzzerInit(void)
{
    GPIO_setAsPeripheralModuleFunctionOutputPin(BUZZER_PORT, BUZZER_PIN,
                                                GPIO_PRIMARY_MODULE_FUNCTION);

    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfigBuzzer);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWM); // start silent
}

void buzzerOn(void)
{
    Timer_A_setCompareValue(TIMER_A0_BASE,
                            TIMER_A_CAPTURECOMPARE_REGISTER_4,
                            upConfigBuzzer.timerPeriod / 2); // ~50% duty
}

void buzzerOff(void)
{
    Timer_A_setCompareValue(TIMER_A0_BASE,
                            TIMER_A_CAPTURECOMPARE_REGISTER_4,
                            0);
}

/* Software clock (time.h + Timer_A1) */

/* Get initial time from host/PC if supported, or fallback to 00:00:00 */
void syncTime(void)
{
    time_t current_time = time(NULL);
    struct tm *t = localtime(&current_time);

    if (t != NULL)
    {
        ClockTime.hour   = t->tm_hour;
        ClockTime.minute = t->tm_min;
        ClockTime.second = t->tm_sec;

        ClockDate.day   = t->tm_mday;
        ClockDate.month = t->tm_mon + 1;
        ClockDate.year  = t->tm_year + 1900;
    }
    else
    {
        ClockTime.hour = ClockTime.minute = ClockTime.second = 0;
        ClockDate.day = 1;
        ClockDate.month = 1;
        ClockDate.year = 2025;
    }
}

void clockInit(void)
{
    /* ACLK from REFO (about 32 kHz) for Timer_A1 */
    CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_4);
    /* 128 kHz / 4 ≈ 32 kHz */

    Timer_A_configureUpMode(TIMER_A1_BASE, &upConfigSecond);
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);

    Interrupt_enableInterrupt(INT_TA1_0);
}

/* Timer_A1 CCR0 ISR: tick software clock and check alarm */
void TA1_0_IRQHandler(void)
{
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
                                         TIMER_A_CAPTURECOMPARE_REGISTER_0);

    /* Tick software time */
    ClockTime.second++;
    if (ClockTime.second >= 60)
    {
        ClockTime.second = 0;
        ClockTime.minute++;
    }
    if (ClockTime.minute >= 60)
    {
        ClockTime.minute = 0;
        ClockTime.hour++;
    }
    if (ClockTime.hour >= 24)
    {
        ClockTime.hour = 0;
        /* Optional: resync from time() once per day */
        syncTime();
    }

    /* Alarm check: use ClockTime as "current" */
    if (g_alarmSet && !g_alarmRinging)
    {
        if (ClockTime.hour   == g_alarmHour &&
            ClockTime.minute == g_alarmMinute &&
            ClockTime.second == 0)
        {
            g_alarmRinging = 1;
            buzzerOn();
        }
    }
}

int main(void)
{
    WDT_A_holdTimer();

    /* 3 MHz DCO for CPU, SMCLK (for buzzer PWM, ADC, etc.) */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_3);
    CS_initClockSignal(CS_MCLK,  CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    LCD_init();
    joystickInit();
    buttonsInit();
    _buzzerInit();
    syncTime();   // initial time
    clockInit();  // Timer_A1 1-second tick

    Interrupt_enableMaster();

    while (1)
    {
        updateFromJoystick();
        checkButtons();

        drawCurrentTime();
        drawAlarmScreen();

        __delay_cycles(24000);  // small delay (~8 ms @ 3 MHz)
    }
}
