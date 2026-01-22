#ifndef ACTIVITY_TRACKER_H_
#define ACTIVITY_TRACKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <ti/grlib/grlib.h>
#include "system_time.h"

#define SAMPLE_RATE_HZ 25
#define REFRACTORY_MS 300 // minimum time between valid steps, prevent double counting
#define PEAK_THRESHOLD 180.0f // peak threshold after filtering
#define STEP_LENGTH_CM 70.0 // avg step length cm
#define GOAL_DISTANCE_CM 5000.0 // goal distance cm

extern volatile uint32_t stepCount;
extern volatile double   distance_cm;
void activity_tracker_init(void);
void activity_tracker_adc_handler(uint64_t status,
                                  uint16_t *conversionValues);

void activity_tracker_timer_tick(void);
void step_counter_task(Graphics_Context *pContext); // activity tracker UI
void step_counter_adc(uint64_t status, uint16_t *conversionValues);
void step_counter_timer(void);

#endif /* ACTIVITY_TRACKER_H_ */
