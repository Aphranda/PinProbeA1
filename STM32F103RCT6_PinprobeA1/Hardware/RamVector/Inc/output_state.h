#ifndef OUTPUT_STATE_H_
#define OUTPUT_STATE_H_

#include <stdbool.h>
#include <stdint.h>

void OutputState_Reset(void);

/* Write a logical output request through the configured LEVEL/PULSE policy. */
bool OutputState_Write(uint8_t output_number, uint8_t requested_level);

/* Release expired physical pulses. Call periodically from the IO task. */
void OutputState_Process(void);

/* Overlay logical state for PULSE points onto the physical output feedback. */
uint16_t OutputState_Forward(uint16_t physical_outputs);

#endif /* OUTPUT_STATE_H_ */
