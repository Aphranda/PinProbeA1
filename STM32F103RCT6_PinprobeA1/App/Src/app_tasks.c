#include <stdbool.h>
#include <stdint.h>

#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

#include "app_tasks.h"
#include "iwdg.h"
#include "BsmRelay.h"
#include "RS485.h"
#include "scpi/scpi.h"
#include "scpi-def.h"
#include "ram_vector.h"
#include "state_vector.h"
#include "cmd_exec.h"
#include "app_log.h"
#include "stm32f1xx_it.h"

#define MODBUS_POLL_DIV 2U
#define IO_RECOVERY_STABLE_SAMPLES 10U

extern osThreadId_t ModBusHandle;
extern osThreadId_t StateVectorHandle;

void SCPITask(void *argument)
{
  (void)argument;

  for (;;)
  {
    uint32_t len;
    const char *buf;

    taskENTER_CRITICAL();
    if (Uart1_RxLength > 3)
    {
      len = Uart1_RxLength;
      buf = (const char *)Uart1_BuffIsReady;
      Uart1_RxLength = 0;
    }
    else
    {
      len = 0;
      buf = NULL;
    }
    taskEXIT_CRITICAL();

    if (buf && len > 0)
    {
      SCPI_Input(&scpi_context, buf, len);
    }

    AppLog_PumpRealtime(4);
    osDelay(10);
  }
}

void ModBusTask(void *argument)
{
  (void)argument;
  osDelay(500);

  uint8_t io_link_ready = 0U;
  uint8_t stable_count = 0U;
  uint8_t pending_in[2] = {0};
  uint8_t pending_out[2] = {0};
  uint8_t has_pending = 0U;

  for (;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

    uint8_t in_buf[2] = {0}, out_buf[2] = {0};
    bool ok = IO_Read(5, 2, in_buf);
    ok = IO_Read(5, 1, out_buf) && ok;
    SetRS485_Ok(ok);

    Vector_IOState_t io;
    (void)RamVector_ReadLocalIO(&io);

    if (!ok) {
      io_link_ready = 0U;
      stable_count = 0U;
      has_pending = 0U;
      io.rs485_ok = VECTOR_IO_LINK_FAULT;
      RamVector_UpdateLocalIO(&io);
      CmdExec_ExecuteAll();
      continue;
    }

    bool zero_frame = (in_buf[0] == 0U && in_buf[1] == 0U &&
                       out_buf[0] == 0U && out_buf[1] == 0U);

    if (zero_frame) {
      io_link_ready = 0U;
      stable_count = 0U;
      has_pending = 0U;
      io.rs485_ok = VECTOR_IO_LINK_RECOVERING;
      RamVector_UpdateLocalIO(&io);
      CmdExec_ExecuteAll();
      continue;
    }

    if (!io_link_ready) {
      if (!has_pending ||
                 pending_in[0] != in_buf[0] || pending_in[1] != in_buf[1] ||
                 pending_out[0] != out_buf[0] || pending_out[1] != out_buf[1]) {
        pending_in[0] = in_buf[0];
        pending_in[1] = in_buf[1];
        pending_out[0] = out_buf[0];
        pending_out[1] = out_buf[1];
        stable_count = 1U;
        has_pending = 1U;
      } else if (stable_count < IO_RECOVERY_STABLE_SAMPLES) {
        stable_count++;
      }

      if (stable_count < IO_RECOVERY_STABLE_SAMPLES) {
        io.rs485_ok = VECTOR_IO_LINK_RECOVERING;
        RamVector_UpdateLocalIO(&io);
        CmdExec_ExecuteAll();
        continue;
      }

      io_link_ready = 1U;
    }

    io.raw_in_lo  = in_buf[0];
    io.raw_in_hi  = in_buf[1];
    io.raw_out_lo = out_buf[0];
    io.raw_out_hi = out_buf[1];
    io.rs485_ok   = VECTOR_IO_LINK_OK;

    if (in_buf[0] & 0x01)      io.door_state = 1;
    else if (in_buf[0] & 0x02) io.door_state = 0;
    else                       io.door_state = 2;

    io.door_moving    = (out_buf[0] & 0x03) ? 1 : 0;
    io.cylinder_cmd[0]= (out_buf[0] & 0x01) ? 1 : 0;
    io.cylinder_cmd[1]= (out_buf[0] & 0x02) ? 1 : 0;
    io.lock_state     = (out_buf[0] & 0x80) ? 1 : 0;

    io.led_state = LED_DecodeState((uint16_t)out_buf[0] | ((uint16_t)out_buf[1] << 8));

    RamVector_UpdateLocalIO(&io);
    CmdExec_ExecuteAll();
  }
}

void WatchDogTask(void *argument)
{
  (void)argument;

  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);
    osDelay(500);
  }
}

void StateVectorTask(void *argument)
{
  (void)argument;
  RamVector_Init(0);
  osDelay(700);

  for (;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StateVector_Input();
  }
}

void SysTimerCallback(void *argument)
{
  static uint8_t modbus_div = 0U;
  (void)argument;

  osThreadFlagsSet(StateVectorHandle, 0x01);

  if (++modbus_div >= MODBUS_POLL_DIV) {
    modbus_div = 0U;
    osThreadFlagsSet(ModBusHandle, 0x01);
  }
}
