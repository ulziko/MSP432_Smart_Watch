#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "alarm.h"
#include "system_time.h"

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

/* Buzzer on P6.6 */
#define BUZZER_TIMER_BASE   TIMER_A0_BASE
#define BUZZER_CCR          TIMER_A_CAPTURECOMPARE_REGISTER_4
#define BUZZER_PORT         GPIO_PORT_P2
#define BUZZER_PIN          GPIO_PIN7
#define BUZZER_HZ           2000u

static uint16_t g_buzzerPeriod = 0;

/* Snooze duration */
const int g_snoozeMinutes = 1;


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

void drawCurrentTime(Graphics_Context *ctx);
void drawAlarmScreen(Graphics_Context *ctx);

int16_t readJoystickX(void);
int16_t readJoystickY(void);
void updateFromJoystick(void);

void buttonsInit(void);
void _buzzerInit(void);
void buzzerOn(void);
void buzzerOff(void);

/* Show current time at top: HH:MM:SS */
void drawCurrentTime(Graphics_Context *ctx)
{
    char buf[16];
    sprintf(buf, "%02d:%02d:%02d",
            ClockTime.hour, ClockTime.minute, ClockTime.second);

    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    GrContextFontSet(ctx, &g_sFontFixed6x8);
    Graphics_drawStringCentered(ctx,
                                (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                64, 8,
                                OPAQUE_TEXT);
}

/* Draw the whole alarm UI: title, picker, and status text */
void drawAlarmScreen(Graphics_Context *ctx)
{
    Graphics_Rectangle band, msgArea, statusArea;
    char buf[32];

    /* Title under the time */
    GrContextFontSet(ctx, &g_sFontFixed6x8);
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    Graphics_drawStringCentered(ctx,
                                (int8_t *)"ALARM",
                                AUTO_STRING_LENGTH,
                                64, 20, OPAQUE_TEXT);

    /* Clear message area (y ~ 24-52) */
    msgArea.xMin = 0;  msgArea.yMin = 24;
    msgArea.xMax = 127; msgArea.yMax = 52;
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_WHITE);
    Graphics_fillRectangle(ctx, &msgArea);

    Graphics_setBackgroundColor(ctx, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    if (g_alarmRinging)
    {
        Graphics_drawStringCentered(ctx,
                                    (int8_t *)"ALARM RINGING!",
                                    AUTO_STRING_LENGTH,
                                    64, 30, OPAQUE_TEXT);
        Graphics_drawStringCentered(ctx,
                                    (int8_t *)"S1=Stop  S2=Snooze",
                                    AUTO_STRING_LENGTH,
                                    64, 42, OPAQUE_TEXT);
    }
    else
    {
        Graphics_drawStringCentered(ctx,
                                    (int8_t *)"S1=Set/Stop",
                                    AUTO_STRING_LENGTH,
                                    64, 42, OPAQUE_TEXT);
    }

    /* Middle selection band */
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_DARK_GRAY);
    band.xMin = 4;
    band.yMin = 64;
    band.xMax = 123;
    band.yMax = 80;
    Graphics_fillRectangle(ctx, &band);

    /* Vertical separator between hour and minute */
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_GRAY);
    Graphics_drawLineV(ctx, 64, 52, 92);

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
    GrContextFontSet(ctx, &g_sFontFixed6x8);
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_GRAY);

    /* Previous hour/minute (top row) */
    sprintf(buf, "%02d", prevHour);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY - offsetY, OPAQUE_TEXT);

    sprintf(buf, "%02d", prevMin);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY - offsetY, OPAQUE_TEXT);

    /* Next hour/minute (bottom row) */
    sprintf(buf, "%02d", nextHour);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY + offsetY, OPAQUE_TEXT);

    sprintf(buf, "%02d", nextMin);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY + offsetY, OPAQUE_TEXT);

    /* Selected hour in middle row */
    if (g_selectedField == 0)
        Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    else
        Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_GRAY);

    sprintf(buf, "%02d", g_hour);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                hourX, centerY, OPAQUE_TEXT);

    /* Selected minute in middle row */
    if (g_selectedField == 1)
        Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    else
        Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_GRAY);

    sprintf(buf, "%02d", g_minute);
    Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                AUTO_STRING_LENGTH,
                                minX, centerY, OPAQUE_TEXT);

    /* Bottom status line area */
    statusArea.xMin = 0;   statusArea.yMin = 100;
    statusArea.xMax = 127; statusArea.yMax = 127;

    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_WHITE);
    Graphics_fillRectangle(ctx, &statusArea);

    Graphics_setBackgroundColor(ctx, GRAPHICS_COLOR_WHITE);
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_BLACK);
    if (g_alarmSet)
    {
        if (g_alarmRinging)
        {
            Graphics_drawStringCentered(ctx,
                                        (int8_t*)"ALARM NOW!",
                                        AUTO_STRING_LENGTH,
                                        64, 110, OPAQUE_TEXT);
        }
        else
        {
            sprintf(buf, "Alarm set: %02d:%02d", g_alarmHour, g_alarmMinute);
            Graphics_drawStringCentered(ctx, (int8_t*)buf,
                                        AUTO_STRING_LENGTH,
                                        64, 110, OPAQUE_TEXT);
        }
    }
    else
    {
        Graphics_drawStringCentered(ctx,
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

void alarm_adc(uint64_t status, uint16_t *conversionValues)
{
    if ((status & ADC_INT1) && conversionValues != NULL)
    {
        g_joyX = conversionValues[0];
        g_joyY = conversionValues[1];
    }
}

void alarm_ta1_handler(void)
{
    if (g_alarmSet && !g_alarmRinging &&
        ClockTime.hour == g_alarmHour &&
        ClockTime.minute == g_alarmMinute &&
        ClockTime.second == 0)
    {
        g_alarmRinging = 1;
        buzzerOn();
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



void alarm_button1_handler(void) // S1
{
    if (g_alarmRinging)
    {
        g_alarmRinging = 0;
        g_alarmSet = 0;
        buzzerOff();
    }
    else
    {
        g_alarmSet = 1;
        g_alarmHour = g_hour;
        g_alarmMinute = g_minute;
    }
}

void alarm_button2_handler(void) // S2
{
    if (!g_alarmRinging) return;

    g_alarmRinging = 0;
    buzzerOff();

    int totalMinutes = ClockTime.hour * 60 + ClockTime.minute + g_snoozeMinutes;
    g_alarmHour   = (totalMinutes / 60) % 24;
    g_alarmMinute = totalMinutes % 60;
    g_alarmSet    = 1;
}

/* Buzzer (Timer_A0 PWM) */

void _buzzerInit(void)
{
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(BUZZER_PORT, BUZZER_PIN,
                                                GPIO_PRIMARY_MODULE_FUNCTION);

    uint32_t smclk = MAP_CS_getSMCLK();
    g_buzzerPeriod = (uint16_t)(smclk / BUZZER_HZ);

    Timer_A_UpModeConfig up = {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        g_buzzerPeriod - 1,
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
        TIMER_A_DO_CLEAR
    };

    MAP_Timer_A_configureUpMode(BUZZER_TIMER_BASE, &up);
    MAP_Timer_A_startCounter(BUZZER_TIMER_BASE, TIMER_A_UP_MODE);

    Timer_A_CompareModeConfig pwm = {
        BUZZER_CCR,
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_RESET_SET,
        0
    };
    MAP_Timer_A_initCompare(BUZZER_TIMER_BASE, &pwm);
}

void buzzerOn(void)
{
    MAP_Timer_A_setCompareValue(BUZZER_TIMER_BASE, BUZZER_CCR, g_buzzerPeriod / 2);
}

void buzzerOff(void)
{
    MAP_Timer_A_setCompareValue(BUZZER_TIMER_BASE, BUZZER_CCR, 0);
}


static uint8_t inited = 0;

void alarm_task(Graphics_Context *ctx)
{
    if (!inited) {
        inited = 1;
        buttonsInit();
        _buzzerInit();
    }

    updateFromJoystick();
    drawCurrentTime(ctx);
    drawAlarmScreen(ctx);
}
