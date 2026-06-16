/*
 * app_log_uart.c - optional realtime UART backend for AppLog.
 */

#include "app_log.h"
#include "stm32f1xx_it.h"

bool AppLog_UartSink(const AppLog_Record_t *record)
{
    char line[128];

    if (Uart1_IsTxBusy()) {
        return false;
    }

    if (AppLog_Format(record, line, sizeof(line)) > 0U) {
        Uart1_Printf("%s\r\n", line);
        return true;
    }

    return false;
}
