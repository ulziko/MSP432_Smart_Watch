#ifndef ACTIVITY_TRACKER_H_
#define ACTIVITY_TRACKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <ti/grlib/grlib.h>
#include "system_time.h"

#define AT_BG_COLOR GRAPHICS_COLOR_WHITE
#define AT_TEXT_COLOR GRAPHICS_COLOR_RED
#define AT_BAR_OUTLINE_COLOR GRAPHICS_COLOR_RED
#define AT_BAR_FILL_COLOR GRAPHICS_COLOR_GREEN
#define AT_DEBUG_COLOR GRAPHICS_COLOR_RED
#define GRAVITY_ALPHA 0.98f

#define SAMPLE_RATE_HZ 25
#define SAMPLE_PERIOD_MS (1000 / SAMPLE_RATE_HZ)
#define REFRACTORY_MS 300 // minimum time between valid steps, prevent double counting
#define PEAK_THRESHOLD 80.0f // peak threshold after filtering 180.0f for standard
#define STEP_LENGTH_CM 3.0 // avg step length 70 cm
#define GOAL_DISTANCE_CM 500.0 // goal distance 500000cm/5km


extern volatile uint32_t stepCount;
extern volatile double   distance_cm;
void activity_tracker_init(void);
void activity_tracker_adc_handler(uint64_t status,
                                  uint16_t *conversionValues);

void activity_tracker_timer_tick(void);
void step_counter_task(Graphics_Context *pContext); // activity tracker UI
void step_counter_reset_screen(void);
void step_counter_adc(uint64_t status, uint16_t *conversionValues);
void step_counter_timer(void);

#endif /* ACTIVITY_TRACKER_H_ */
