#include "io_probe.h"

static IOProbeMode_t probe_mode = IO_PROBE_RAW;

void IOProbe_Reset(void)
{
    probe_mode = IO_PROBE_RAW;
}

uint8_t IOProbe_SetMode(IOProbeMode_t mode)
{
    if (mode != IO_PROBE_RAW && mode != IO_PROBE_FORWARD) {
        return 0U;
    }

    probe_mode = mode;
    return 1U;
}

IOProbeMode_t IOProbe_GetMode(void)
{
    return probe_mode;
}
