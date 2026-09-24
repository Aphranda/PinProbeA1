#include "output_state.h"

#include "flash.h"
#include "RS485.h"
#include "tim.h"

static uint16_t logical_outputs;
static uint16_t pulse_active;
static uint16_t applied_pulse_mask;
static uint32_t pulse_deadline[FLASH_OUTPUT_COUNT];

static bool OutputState_TimeReached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

void OutputState_Reset(void)
{
    uint8_t index;

    logical_outputs = 0U;
    pulse_active = 0U;
    applied_pulse_mask = Flash_GetOutputPulseMask();
    for (index = 0U; index < FLASH_OUTPUT_COUNT; ++index) {
        pulse_deadline[index] = 0U;
    }
}

bool OutputState_Write(uint8_t output_number, uint8_t requested_level)
{
    uint8_t index;
    uint16_t bit;
    uint16_t pulse_mask;
    uint16_t pulse_width_ms;

    if (output_number == 0U || output_number > FLASH_OUTPUT_COUNT ||
        requested_level > 1U) {
        return false;
    }

    index = (uint8_t)(output_number - 1U);
    bit = (uint16_t)(1U << index);
    pulse_mask = Flash_GetOutputPulseMask();

    if (requested_level == 0U) {
        logical_outputs &= (uint16_t)~bit;
        pulse_active &= (uint16_t)~bit;
        pulse_deadline[index] = 0U;
        return WriteIO(output_number, 0U);
    }

    if ((pulse_mask & bit) == 0U) {
        logical_outputs |= bit;
        pulse_active &= (uint16_t)~bit;
        pulse_deadline[index] = 0U;
        return WriteIO(output_number, 1U);
    }

    /* PULSE is edge-triggered: a repeated high request does not extend it. */
    if ((logical_outputs & bit) != 0U) {
        return true;
    }

    pulse_width_ms = Flash_GetOutputPulseWidth();
    if (!WriteIO(output_number, 1U)) {
        return false;
    }

    logical_outputs |= bit;
    pulse_active |= bit;
    pulse_deadline[index] = GetTim1Ms() + pulse_width_ms;
    return true;
}

void OutputState_Process(void)
{
    uint16_t pulse_mask = Flash_GetOutputPulseMask();
    uint16_t changed_mask = (uint16_t)(pulse_mask ^ applied_pulse_mask);
    uint16_t pulse_width_ms = Flash_GetOutputPulseWidth();
    uint32_t now = GetTim1Ms();
    uint8_t index;

    for (index = 0U; index < FLASH_OUTPUT_COUNT; ++index) {
        uint16_t bit = (uint16_t)(1U << index);

        if ((changed_mask & bit) != 0U) {
            if ((pulse_mask & bit) == 0U) {
                pulse_active &= (uint16_t)~bit;
                pulse_deadline[index] = 0U;
                (void)WriteIO((uint8_t)(index + 1U),
                              (logical_outputs & bit) != 0U ? 1U : 0U);
            } else if ((logical_outputs & bit) != 0U) {
                pulse_active |= bit;
                pulse_deadline[index] = now + pulse_width_ms;
                (void)WriteIO((uint8_t)(index + 1U), 1U);
            }
        }

        if ((pulse_active & bit) != 0U &&
            OutputState_TimeReached(now, pulse_deadline[index])) {
            if (WriteIO((uint8_t)(index + 1U), 0U)) {
                pulse_active &= (uint16_t)~bit;
                pulse_deadline[index] = 0U;
            }
        }
    }

    applied_pulse_mask = pulse_mask;
}

uint16_t OutputState_Forward(uint16_t physical_outputs)
{
    uint16_t pulse_mask = Flash_GetOutputPulseMask();

    return (uint16_t)((physical_outputs & (uint16_t)~pulse_mask) |
                      (logical_outputs & pulse_mask));
}
