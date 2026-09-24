#ifndef IO_PROBE_H_
#define IO_PROBE_H_

#include <stdint.h>

typedef enum {
    IO_PROBE_RAW = 0U,
    IO_PROBE_FORWARD = 1U,
} IOProbeMode_t;

void IOProbe_Reset(void);
uint8_t IOProbe_SetMode(IOProbeMode_t mode);
IOProbeMode_t IOProbe_GetMode(void);

#endif /* IO_PROBE_H_ */
