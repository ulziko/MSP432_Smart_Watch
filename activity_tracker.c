#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "activity_tracker.h"
#include "system_time.h"
#include "tasks.h"

static Graphics_Context *ctx = NULL;
static bool step_screen_init = false;


volatile uint32_t stepCount = 0;
volatile double   distance_cm = 0.0;
volatile uint32_t lastStepTime = 0;

static uint32_t timeMs = 0;
static float gx = 0, gy = 0, gz = 0;

typedef enum {
    STEP_IDLE,
    STEP_ARMED
} step_state_t;

static step_state_t stepState = STEP_IDLE;
static float peakValue = 0.0f;

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

void graphicsInit_once(Graphics_Context *pContext) {
    Graphics_initContext(pContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(pContext, AT_TEXT_COLOR);
    Graphics_setBackgroundColor(pContext, AT_BG_COLOR);
    GrContextFontSet(pContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(pContext);

    Graphics_drawStringCentered(pContext,
        (int8_t *)"steps",
        AUTO_STRING_LENGTH,
        64, 90,
        OPAQUE_TEXT);
}

void updateProgressBar(double percent) {
    if (!ctx) return;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    Graphics_Rectangle area = {0, 0, 127, 50};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &area);

    Graphics_Rectangle outline = {20, 20, 108, 35};
    Graphics_setForegroundColor(ctx, AT_BAR_OUTLINE_COLOR);
    Graphics_drawRectangle(ctx, &outline);

    int fill = (int)(percent * (108 - 20) / 100.0);
    Graphics_Rectangle bar = {20, 20, 20 + fill, 35};
    Graphics_setForegroundColor(ctx, AT_BAR_FILL_COLOR);
    Graphics_fillRectangle(ctx, &bar);

    char txt[12];
    sprintf(txt, "%.0f%%", percent);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(ctx, (int8_t*)txt,
        AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);
}

void updateStepCountText(void) {
    if (!ctx) return;

    Graphics_Rectangle area = {30, 60, 100, 80};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &area);

    char txt[16];
    sprintf(txt, "Steps: %lu", (unsigned long)stepCount);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(ctx,
        (int8_t*)txt,
        AUTO_STRING_LENGTH,
        64, 70,
        OPAQUE_TEXT);
}

void updateDebug(float signal) {
    if (!ctx) return;

    Graphics_Rectangle area = {0, 104, 127, 128};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &area);

    char txt[40];
    sprintf(txt, "S:%.1f %s",
            fabsf(signal),
            (stepState == STEP_ARMED) ? "ARM" : "IDLE");

    Graphics_setForegroundColor(ctx, AT_DEBUG_COLOR);
    Graphics_drawStringCentered(ctx,
        (int8_t*)txt,
        AUTO_STRING_LENGTH,
        64, 112,
        OPAQUE_TEXT);
}

void activity_tracker_init(void) {
    stepCount = 0;
    distance_cm = 0;
    lastStepTime = 0;
    timeMs = 0;
    gx = gy = gz = 0;
    stepState = STEP_IDLE;
}

void activity_tracker_adc_handler(uint64_t status, uint16_t *v)
{
    if (current_task != STEP_COUNTER) return;

    if (!(status & ADC_INT1)) return;

    float x = (float)v[0];
    float y = (float)v[1];
    float z = (float)v[2];

    gx = GRAVITY_ALPHA * gx + (1.0f - GRAVITY_ALPHA) * x;
    gy = GRAVITY_ALPHA * gy + (1.0f - GRAVITY_ALPHA) * y;
    gz = GRAVITY_ALPHA * gz + (1.0f - GRAVITY_ALPHA) * z;

    x -= gx; y -= gy; z -= gz;

    float mag = sqrtf(x*x + y*y + z*z);
    float signal = bandpassFilter(mag);

    uint32_t now = getTimeMs();
    bool stepDetected = false;

    switch (stepState) {

    case STEP_IDLE:
        if (signal > PEAK_THRESHOLD &&
            (now - lastStepTime) > REFRACTORY_MS) {
            stepState = STEP_ARMED;
            peakValue = signal;
        }
        break;

    case STEP_ARMED:
        if (signal > peakValue)
            peakValue = signal;

        if (signal < 0.0f) {
            stepCount++;
            distance_cm = stepCount * STEP_LENGTH_CM;
            lastStepTime = now;
            stepState = STEP_IDLE;
            stepDetected = true;

            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        }
        break;
    }

    if (ctx) {
        updateDebug(signal);
        if (stepDetected) {
            double p = (distance_cm / GOAL_DISTANCE_CM) * 100.0;
            updateStepCountText();
            updateProgressBar(p);
        }
    }
}

void activity_tracker_timer_tick(void)
{
    if (current_task != STEP_COUNTER)
        return;

    timeMs += SAMPLE_PERIOD_MS;

    if (ClockTime.hour == 0 &&
        ClockTime.minute == 0 &&
        ClockTime.second == 0) {
        stepCount = 0;
        distance_cm = 0;
    }
}

void step_counter_task(Graphics_Context *pContext)
{
    if (current_task != STEP_COUNTER)
        return;

    if (!step_screen_init) {
        ctx = pContext;

        Graphics_setForegroundColor(ctx, AT_BG_COLOR);
        Graphics_setBackgroundColor(ctx, AT_BG_COLOR);
        Graphics_clearDisplay(ctx);

        graphicsInit_once(ctx);
        updateStepCountText();
        updateProgressBar(0.0);

        step_screen_init = true;
    }
}

void step_counter_reset_screen(void) {
    step_screen_init = false;
    ctx = NULL;
}
