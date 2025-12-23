#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <math.h>
#include <stdio.h>
#include "activity_tracker.h"

static Graphics_Context *ctx = NULL;

volatile uint32_t stepCount     = 0;
volatile double   distance_cm   = 0.0;
volatile uint32_t lastStepTime  = 0;
volatile bool     stepDetected  = false;

float   magBuffer[WINDOW_SIZE] = {0};
uint32_t bufferIndex = 0;

static uint32_t timeMs = 0;
static inline uint32_t getTimeMs(void) {
    return timeMs;
}

static inline float bandpassFilter(float input) {
    static float x1=0, x2=0, y1=0, y2=0;
    const float b0=0.2066f, b1=0.0f, b2=-0.2066f;
    const float a1=-1.3695f, a2=0.5868f;

    float y = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    x2 = x1; x1 = input;
    y2 = y1; y1 = y;
    return y;
}

static void graphicsInit_once(Graphics_Context *pContext) {
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(pContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(pContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(pContext, GRAPHICS_COLOR_BLACK);
    GrContextFontSet(pContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(pContext);
}

//progress bar
void drawStepData(double magnitude, double thresholdHigh) {
    if (!ctx) return;

    Graphics_clearDisplay(ctx);

    double progress = 0.0;
    if (GOAL_DISTANCE_CM > 0.0)
        progress = (distance_cm / GOAL_DISTANCE_CM) * 100.0;
    if (progress < 0.0) progress = 0.0;
    if (progress > 100.0) progress = 100.0;

    Graphics_Rectangle outline = (Graphics_Rectangle){20, 20, 108, 35};
    Graphics_drawRectangle(ctx, &outline);

    int fillWidth = (int)(progress * (108 - 20) / 100.0);
    Graphics_Rectangle fill = (Graphics_Rectangle){20, 20, 20 + fillWidth, 35};
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_GREEN);
    Graphics_fillRectangle(ctx, &fill);

    char percentStr[12];
    sprintf(percentStr, "%.1f%%", progress);
    Graphics_setForegroundColor(ctx, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(ctx, (int8_t *)percentStr, AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);

    /* Steps count */
    char stepsStr[16];
    sprintf(stepsStr, "%lu", (unsigned long)stepCount);
    Graphics_drawStringCentered(ctx, (int8_t *)stepsStr, AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);
    Graphics_drawStringCentered(ctx, (int8_t *)"steps", AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);

    /* Debug line */
    char debugStr[32];
    sprintf(debugStr, "Mag:%.0f Th:%.0f", magnitude, thresholdHigh);
    Graphics_drawStringCentered(ctx, (int8_t *)debugStr, AUTO_STRING_LENGTH, 64, 104, OPAQUE_TEXT);
}

//ADC handler
void step_counter_adc(uint64_t status, uint16_t *conversionValues) {
    if (!(status & ADC_INT2)) return;

    float x = (float)conversionValues[0];
    float y = (float)conversionValues[1];
    float z = (float)conversionValues[2];

    //static offsets to remove gravity bias
    static float ox=8000.0f, oy=8000.0f, oz=8000.0f;
    x -= ox; y -= oy; z -= oz;

    float magnitude = sqrtf(x*x + y*y + z*z);

    //peak detection
    float filtered = bandpassFilter(magnitude);
    float peak = fabsf(filtered);

    uint32_t now = getTimeMs();
    if (peak > PEAK_THRESHOLD && (now - lastStepTime) > REFRACTORY_MS) {
        stepCount++;
        distance_cm = stepCount * STEP_LENGTH_CM;
        lastStepTime = now;
        stepDetected = true;
    } else {
        stepDetected = false;
    }

    drawStepData(filtered, PEAK_THRESHOLD);
}

//sampling timebase and sleep cadence
void step_counter_timer(void) {
    /* ~40 ms per sample at 25 Hz */
    timeMs += 40;
}

//main task function
void step_counter_task(Graphics_Context *pContext) {
    if (ctx == NULL) {
        ctx = pContext;
        graphicsInit_once(ctx);
        Graphics_clearDisplay(ctx);
        drawStepData(0.0, PEAK_THRESHOLD);
    }

    //sleep until next interrupt (ADC or timer)
    PCM_gotoLPM0();
}
