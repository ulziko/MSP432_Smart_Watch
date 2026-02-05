#ifndef ACTIVITY_TRACKER_H_
#define ACTIVITY_TRACKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <ti/grlib/grlib.h>
#include "system_time.h"

#define AT_BG_COLOR            0x00363E69
#define AT_TEXT_COLOR          GRAPHICS_COLOR_WHITE
#define AT_BAR_OUTLINE_COLOR   GRAPHICS_COLOR_WHITE
#define AT_BAR_FILL_COLOR      GRAPHICS_COLOR_WHITE
#define AT_DEBUG_COLOR         GRAPHICS_COLOR_WHITE

#define GRAVITY_ALPHA      0.98f
#define SAMPLE_RATE_HZ     25
#define SAMPLE_PERIOD_MS   (1000 / SAMPLE_RATE_HZ)

#ifdef SMCLK_FREQUENCY
#define SAMPLE_PERIOD_TICKS  ((SMCLK_FREQUENCY / 64) / SAMPLE_RATE_HZ)
#endif

#define REFRACTORY_MS      300
#define PEAK_THRESHOLD     8.0f
#define STEP_LENGTH_CM     3.0
#define GOAL_DISTANCE_CM   500.0

extern volatile uint32_t stepCount;
extern volatile double   distance_cm;

void activity_tracker_init(void);

void activity_tracker_adc_handler(uint64_t status,
                                  uint16_t *conversionValues);

void activity_tracker_timer_tick(void);   // TA1 handler

void step_counter_task(Graphics_Context *pContext);
void step_counter_reset_screen(void);
void step_counter_button_handler(void);

#endif /* ACTIVITY_TRACKER_H_ */
