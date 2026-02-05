#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "activity_tracker.h"
#include "tasks.h"
#include "system_time.h"

static Graphics_Context *ctx = NULL;
static bool screen_init = false;
static volatile bool redraw = false;

/* step data */
volatile uint32_t stepCount = 0;
volatile double distance_cm = 0;
static uint32_t lastStepTime = 0;
static uint32_t timeMs = 0;

/* accelerometer state */
static float gx = 0, gy = 0, gz = 0;

typedef enum {
    STEP_IDLE,
    STEP_ARMED
} step_state_t;

static step_state_t stepState = STEP_IDLE;
static float peakValue = 0.0f;

/* filtering threshold value */
static inline float bandpassFilter(float input)
{
    static float x1=0, x2=0, y1=0, y2=0;
    const float b0=0.2066f, b1=0.0f, b2=-0.2066f;
    const float a1=-1.3695f, a2=0.5868f;

    float y = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    x2 = x1; x1 = input;
    y2 = y1; y1 = y;
    return y;
}

static void draw_static_ui(void)
{
    Graphics_clearDisplay(ctx);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_setBackgroundColor(ctx, AT_BG_COLOR);
    GrContextFontSet(ctx, &g_sFontFixed6x8);
}

static void draw_dynamic_ui(void)
{
    double percent = (distance_cm / GOAL_DISTANCE_CM) * 100.0;
    if (percent > 100) percent = 100;

    /* progress bar background */
    Graphics_Rectangle bar_bg = {0, 0, 127, 50};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &bar_bg);

    /* outline */
    Graphics_Rectangle outline = {20, 20, 108, 35};
    Graphics_setForegroundColor(ctx, AT_BAR_OUTLINE_COLOR);
    Graphics_drawRectangle(ctx, &outline);

    /* fill */
    int fill = (int)(percent * (108 - 20) / 100.0);
    Graphics_Rectangle bar = {20, 20, 20 + fill, 35};
    Graphics_setForegroundColor(ctx, AT_BAR_FILL_COLOR);
    Graphics_fillRectangle(ctx, &bar);

    /* percentage text */
    char buf[16];
    sprintf(buf, "%.0f%%", percent);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(
        ctx, (int8_t*)buf,
        AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);

    /* step count background */
    Graphics_Rectangle steps_bg = {0, 60, 127, 80};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &steps_bg);

    sprintf(buf, "Steps: %lu", stepCount);
    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(
        ctx, (int8_t*)buf,
        AUTO_STRING_LENGTH, 64, 70, OPAQUE_TEXT);
}

static volatile float last_signal = 0.0f;

static void draw_debug(float signal)
{
    Graphics_Rectangle dbg = {0, 90, 127, 127};
    Graphics_setForegroundColor(ctx, AT_BG_COLOR);
    Graphics_fillRectangle(ctx, &dbg);

    char buf[20];
    sprintf(buf, "sig: %.2f", signal);

    Graphics_setForegroundColor(ctx, AT_TEXT_COLOR);
    Graphics_drawStringCentered(
        ctx, (int8_t*)buf,
        AUTO_STRING_LENGTH, 64, 108, OPAQUE_TEXT);
}

void step_counter_task(Graphics_Context *pContext)
{
    if (current_task != STEP_COUNTER)
        return;

    if (!screen_init) {
        ctx = pContext;

        Graphics_setForegroundColor(ctx, AT_BG_COLOR);
        Graphics_setBackgroundColor(ctx, AT_BG_COLOR);

        Graphics_clearDisplay(ctx);
        draw_static_ui();

        screen_init = true;
        redraw = true;
    }

    if (redraw) {
        draw_dynamic_ui();
        draw_debug(last_signal);
        redraw = false;
    }
}

void step_counter_reset_screen(void)
{
    screen_init = false;
    ctx = NULL;
}

void activity_tracker_adc_handler(uint64_t status, uint16_t *v)
{
    if (current_task != STEP_COUNTER) return;
    if (!(status & ADC_INT1)) return;

    timeMs += SAMPLE_PERIOD_MS;

    float x = v[0], y = v[1], z = v[2];

    gx = GRAVITY_ALPHA * gx + (1.0f - GRAVITY_ALPHA) * x;
    gy = GRAVITY_ALPHA * gy + (1.0f - GRAVITY_ALPHA) * y;
    gz = GRAVITY_ALPHA * gz + (1.0f - GRAVITY_ALPHA) * z;

    x -= gx; y -= gy; z -= gz;

    float mag = sqrtf(x*x + y*y + z*z);
    float signal = bandpassFilter(mag);
    last_signal = signal;

    uint32_t now = timeMs;

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
            redraw = true;
        }
        break;
    }
}

void activity_tracker_timer_tick(void)
{
    /* daily reset step count and distance at midnight */
    if (ClockTime.hour == 0 &&
        ClockTime.minute == 0 &&
        ClockTime.second == 0) {
        stepCount = 0;
        distance_cm = 0;
        redraw = true;
    }
}

void step_counter_button_handler(void)
{
    step_counter_reset_screen();
    current_task = MAIN_PAGE;
}
