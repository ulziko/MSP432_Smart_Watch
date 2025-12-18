#ifndef ACTIVITY_TRACKER_H_
#define ACTIVITY_TRACKER_H_

#include <ti/grlib/grlib.h>
#include <stdint.h>
#include <stdbool.h>

#define SAMPLE_RATE 25
#define REFRACTORY_MS 300 //ms between steps
#define PEAK_THRESHOLD 3.0f // filter peak threshold
#define STEP_LENGTH_CM 70.0 // average step length
#define GOAL_DISTANCE_CM 5000.0 // progress bar goal
#define WINDOW_SIZE 64

extern volatile uint32_t stepCount;
extern volatile double   distance_cm;

extern volatile uint32_t lastStepTime;
extern volatile bool     stepDetected;

extern float   magBuffer[WINDOW_SIZE];
extern uint32_t bufferIndex;

// handlers for tasks.h
void step_counter_task(Graphics_Context *pContext);
void step_counter_adc(uint64_t status, uint16_t *conversionValues);
void step_counter_timer(void);

void drawStepData(double magnitude, double thresholdHigh);

#endif /* ACTIVITY_TRACKER_H_ */
