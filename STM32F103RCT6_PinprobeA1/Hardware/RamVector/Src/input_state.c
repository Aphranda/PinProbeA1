#include "input_state.h"

static uint16_t pulse_latched;
static uint16_t pulse_raw_previous;
static uint16_t forwarded_inputs;

void InputState_Reset(void)
{
    pulse_latched = 0U;
    pulse_raw_previous = 0U;
    forwarded_inputs = 0U;
}

uint16_t InputState_GetForwarded(void)
{
    return forwarded_inputs;
}

uint16_t InputState_Forward(uint16_t raw_inputs,
                            uint16_t pulse_mask,
                            uint16_t set_mask,
                            uint16_t clear_mask)
{
    uint16_t pulse_raw = (uint16_t)(raw_inputs & pulse_mask);
    uint16_t rising_edges = (uint16_t)(pulse_raw & (uint16_t)~pulse_raw_previous);
    uint16_t managed_pulse_mask = (uint16_t)(pulse_mask & (set_mask | clear_mask));

    /* A PULSE point is edge-driven, with explicit application paths. */
    pulse_latched &= managed_pulse_mask;
    pulse_latched |= (uint16_t)(rising_edges & set_mask & managed_pulse_mask);
    pulse_latched &= (uint16_t)~(rising_edges & clear_mask & managed_pulse_mask);
    pulse_raw_previous = pulse_raw;

    /* Points without an application path retain raw semantics. */
    forwarded_inputs = (uint16_t)((raw_inputs & (uint16_t)~managed_pulse_mask) |
                                  pulse_latched);
    return forwarded_inputs;
}
