#ifndef INPUT_STATE_H_
#define INPUT_STATE_H_

#include <stdint.h>

void InputState_Reset(void);
uint16_t InputState_GetForwarded(void);

/*
 * LEVEL inputs follow raw_inputs. PULSE inputs toggle their latched state on
 * debounced rising edges, but only when the application supplies the bit in
 * set_mask or clear_mask. This keeps capture and release paths explicit.
 */
uint16_t InputState_Forward(uint16_t raw_inputs,
                            uint16_t pulse_mask,
                            uint16_t set_mask,
                            uint16_t clear_mask);

#endif /* INPUT_STATE_H_ */
