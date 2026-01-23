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

#define AT_BG_COLOR GRAPHICS_COLOR_WHITE
#define AT_TEXT_COLOR GRAPHICS_COLOR_RED
#define AT_BAR_OUTLINE_COLOR GRAPHICS_COLOR_RED
#define AT_BAR_FILL_COLOR GRAPHICS_COLOR_GREEN
#define AT_DEBUG_COLOR GRAPHICS_COLOR_CYAN
#define PEAK_THRESHOLD 180.0f
#define REFRACTORY_MS 300
#define STEP_LENGTH_CM 70.0
#define GOAL_DISTANCE_CM 5000.0
#define GRAVITY_ALPHA 0.98f

static Graphics_Context *ctx = NULL;
volatile uint32_t stepCount = 0;
volatile double distance_cm = 0.0;
volatile uint32_t lastStepTime = 0;
static uint32_t timeMs = 0;
static float gx = 0, gy = 0, gz = 0; // gravity estimate

typedef enum {
    STEP_IDLE,
    STEP_ARMED
} StepState;

static StepState stepState = STEP_IDLE;
static float peakValue = 0.0f;

static inline uint32_t getTimeMs(void) {
    return timeMs;
}

// sample 25 Hz
static inline float bandpassFilter(float input) {
    static float x1=0, x2=0, y1=0, y2=0;
    const float b0=0.2066f, b1=0.0f, b2=-0.2066f;
    const float a1=-1.3695f, a2=0.5868f;

    float y = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    x2 = x1; x1 = input;
    y2 = y1; y1 = y;
    return y;
}

// progress bar display
static void graphicsInit_once(Graphics_Context *pContext) {
    Graphics_initContext(pContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(pContext, AT_TEXT_COLOR);
    Graphics_setBackgroundColor(pContext, AT_BG_COLOR);
    GrContextFontSet(pContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(pContext);

    Graphics_drawStringCentered(pContext, (int8_t*)"steps",
        AUTO_STRING_LENGTH, 64, 90, OPAQUE_TEXT);
}

static void updateProgressBar(double percent) {
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

static void updateStepText(void) {
    if (!ctx) return;

    Graphics_Rectangle area = {30, 60, 100, 80};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &area);

    char txt[16];
    sprintf(txt, "%lu", (unsigned long)stepCount);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(ctx, (int8_t*)txt, AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);
}

static void updateDebug(float signal) {
    if (!ctx) return;

    Graphics_Rectangle area = {0, 104, 127, 128};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &area);

    char txt[32];
    sprintf(txt, "Sig:%.1f Th:%.0f", fabsf(signal), PEAK_THRESHOLD);
    Graphics_setForegroundColor(ctx, AT_DEBUG_COLOR);
    Graphics_drawStringCentered(ctx, (int8_t*)txt, AUTO_STRING_LENGTH, 64, 112, OPAQUE_TEXT);
}

// step detection
void activity_tracker_init(void) {
    stepCount = 0;
    distance_cm = 0;
    lastStepTime = 0;
    timeMs = 0;
    gx = gy = gz = 0;
    stepState = STEP_IDLE;
}

void activity_tracker_adc_handler(uint64_t status, uint16_t *adc) {
    if (!(status & ADC_INT2)) return;

    float x = (float)adc[0];
    float y = (float)adc[1];
    float z = (float)adc[2];

    gx = GRAVITY_ALPHA * gx + (1 - GRAVITY_ALPHA) * x; // gravity removal
    gy = GRAVITY_ALPHA * gy + (1 - GRAVITY_ALPHA) * y;
    gz = GRAVITY_ALPHA * gz + (1 - GRAVITY_ALPHA) * z;

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
        }
        break;
    }

    if (ctx && stepDetected) {
        double prog = (distance_cm / GOAL_DISTANCE_CM) * 100.0;
        updateProgressBar(prog);
        updateStepText();
        updateDebug(signal);
    }
}


void activity_tracker_timer_tick(void) {
    timeMs += 1000;

    if (ClockTime.hour == 0 &&
        ClockTime.minute == 0 &&
        ClockTime.second == 0) {
        stepCount = 0;
        distance_cm = 0;
    }
}

void step_counter_task(Graphics_Context *pContext) {
    if (!ctx) {
        ctx = pContext;
        graphicsInit_once(ctx);
        updateProgressBar(0);
        updateStepText();
    }
    PCM_gotoLPM0();
}

void step_counter_adc(uint64_t s, uint16_t *v) {(void)s;(void)v;}
void step_counter_timer(void) {}
