/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "iwdg.h"
#include "ram_vector.h"
#include "event_table.h"
#include "cmd_exec.h"
#include "BsmRelay.h"
#include "tim.h"
#include "flash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SCPI */
osThreadId_t SCPIHandle;
const osThreadAttr_t SCPI_attributes = {
  .name = "SCPI",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ModBus */
osThreadId_t ModBusHandle;
const osThreadAttr_t ModBus_attributes = {
  .name = "ModBus",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for WatchDog */
osThreadId_t WatchDogHandle;
const osThreadAttr_t WatchDog_attributes = {
  .name = "WatchDog",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for StateVector */
osThreadId_t StateVectorHandle;
const osThreadAttr_t StateVector_attributes = {
  .name = "StateVector",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SCPITimer */
osTimerId_t SCPITimerHandle;
const osTimerAttr_t SCPITimer_attributes = {
  .name = "SCPITimer"
};
/* Definitions for COMMutex */
osMutexId_t COMMutexHandle;
const osMutexAttr_t COMMutex_attributes = {
  .name = "COMMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void SCPITask(void *argument);
void ModBusTask(void *argument);
void WatchDogTask(void *argument);
void StateVectorTask(void *argument);
void SCPITimerCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of COMMutex */
  COMMutexHandle = osMutexNew(&COMMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of SCPITimer */
  SCPITimerHandle = osTimerNew(SCPITimerCallback, osTimerPeriodic, NULL, &SCPITimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of SCPI */
  SCPIHandle = osThreadNew(SCPITask, NULL, &SCPI_attributes);

  /* creation of ModBus */
  ModBusHandle = osThreadNew(ModBusTask, NULL, &ModBus_attributes);

  /* creation of WatchDog */
  WatchDogHandle = osThreadNew(WatchDogTask, NULL, &WatchDog_attributes);

  /* creation of StateVector */
  StateVectorHandle = osThreadNew(StateVectorTask, NULL, &StateVector_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_SCPITask */
/**
* @brief Function implementing the SCPI thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SCPITask */
__weak void SCPITask(void *argument)
{
  /* USER CODE BEGIN SCPITask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END SCPITask */
}

/* USER CODE BEGIN Header_ModBusTask */
/**
* @brief Function implementing the ModBus thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ModBusTask */
__weak void ModBusTask(void *argument)
{
  /* USER CODE BEGIN ModBusTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ModBusTask */
}

/* USER CODE BEGIN Header_WatchDogTask */
/**
* @brief Function implementing the WatchDog thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_WatchDogTask */
__weak void WatchDogTask(void *argument)
{
  /* USER CODE BEGIN WatchDogTask */
  /* Infinite loop */
  for(;;)
  {
    HAL_IWDG_Refresh(&hiwdg); // 独立看门狗喂狗，超时约4秒
    osDelay(500);             // 每500ms喂一次，留足余量
  }
  /* USER CODE END WatchDogTask */
}

/* USER CODE BEGIN Header_StateVectorTask */
/**
* @brief Function implementing the StateVector thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StateVectorTask */
__weak void StateVectorTask(void *argument)
{
  /* USER CODE BEGIN StateVectorTask */
  RamVector_Init(1);

  /* 状态机局部变量 (从 StateMachine.c 迁移) */
  uint32_t lock_press_tick = 0, lock_release_tick = 0;
  uint8_t  lock_released = 1;
  uint32_t door_ready_tick = 0, door_open_confirm_tick = 0, door_close_confirm_tick = 0;
  uint32_t release_start_tick = 0;
  uint32_t door_close_start_tick = 0, door_close_done_tick = 0, door_open_start_tick = 0;
  uint32_t door_close_default_ms = 2500, air_last_check_tick = 0;
  uint8_t  door_close_timing = 0, door_close_from_full = 0, door_close_time_learned = 0;
  uint8_t  poweron_position_ok = 0;
  uint8_t  door_up_cnt = 0, door_down_cnt = 0, door_up_db = 0, door_down_db = 0;
  uint8_t  system_status = V_STATE_INIT;
  uint8_t  rs485_err_cnt = 0;
  #define LOCK_PRESS_MS    300
  #define LOCK_IDLE_MS     1000
  #define DOOR_READY_MS    200
  #define DOOR_CLOSE_CONFIRM_MS 500
  #define DOOR_OPEN_CONFIRM_MS 200
  #define RELEASE_DELAY_MS 200
  #define DOOR_DEBOUNCE_CNT 3

  osDelay(500); /* 等待 IO 板启动 */

  for(;;)
  {
    /* ── 1. 读 IO ── */
    uint8_t in_buf[2] = {0}, out_buf[2] = {0};
    bool io_ok = IO_Read(5, 2, in_buf);
    io_ok = IO_Read(5, 1, out_buf) && io_ok;
    SetRS485_Ok(io_ok);
    if (!io_ok) {
        if (++rs485_err_cnt >= 40) { rs485_err_cnt = 0; }
        osDelay(50); continue;
    }

    uint8_t in_01_08 = in_buf[0], in_09_16 = in_buf[1];
    uint8_t out_01_08 = out_buf[0], out_09_16 = out_buf[1];

    /* ── 2. 消抖 ── */
    if (in_01_08 & 0x01) { if (++door_up_cnt >= DOOR_DEBOUNCE_CNT) door_up_db = 1; door_down_cnt = 0; }
    else { door_up_cnt = 0; door_up_db = 0; }
    if (in_01_08 & 0x02) { if (++door_down_cnt >= DOOR_DEBOUNCE_CNT) door_down_db = 1; door_up_cnt = 0; }
    else { door_down_cnt = 0; door_down_db = 0; }
    if (door_up_db)   in_01_08 |= 0x01; else in_01_08 &= ~0x01;
    if (door_down_db) in_01_08 |= 0x02; else in_01_08 &= ~0x02;

    /* ── 3. 更新 IO 镜像 ── */
    Vector_IOState_t io;
    memset(&io, 0, sizeof(io));
    io.cylinder_cmd[0] = (out_01_08 & 0x01) ? 1 : ((out_01_08 & 0x02) ? 0 : 2);
    io.door_state = (in_01_08 & 0x01) ? 1 : ((in_01_08 & 0x02) ? 0 : 2);
    io.door_moving = (out_01_08 & 0x03) ? 1 : 0;
    io.sensor_summary = in_01_08;
    io.led_state = (out_01_08 >> 4) & 0x07;
    io.lock_state = (out_01_08 & 0x80) ? 1 : 0;
    RamVector_UpdateLocalIO(&io);

    /* ── 4. 收集事件 + 查表 → 执行 ── */
    uint32_t now = GetTim1Ms();
    Vector_Cmd_t vcmd = VCMD_NONE;

    /* 急停按钮 (任意状态优先) */
    bool estop;
    if (Flash_GetEstopType() == 1)
        estop = ((in_09_16 & 0x08) != 0);
    else
        estop = ((in_09_16 & 0x08) == 0);
    if (estop) {
        system_status = V_STATE_EMERGENCY;
        if (!(in_01_08 & 0x01)) CmdExec_Execute(VCMD_CYLINDER_OPEN);
        CmdExec_Execute(VCMD_LOCK);
        CmdExec_Execute(VCMD_LED_RED);
        door_close_done_tick = 0;
    }
    /* 激光传感器 → 超时检测 */
    else if ((in_01_08 & 0xE0) || (in_09_16 & 0x01)) {
        uint32_t door_elapsed = (door_close_start_tick) ? (now - door_close_start_tick) : 0;
        if (door_close_time_learned && door_close_timing && door_close_start_tick &&
            !(in_01_08 & 0x02) && (door_elapsed > door_close_default_ms * 2 / 3)) {
            system_status = V_STATE_EMERGENCY;
            CmdExec_Execute(VCMD_CYLINDER_OPEN);
            CmdExec_Execute(VCMD_LOCK);
            CmdExec_Execute(VCMD_LED_RED);
            door_close_start_tick = 0; door_close_timing = 0; door_close_done_tick = 0;
        }
    }
    /* 正常状态流转 */
    else {
        /* Emergency 恢复 */
        if (system_status == V_STATE_EMERGENCY) {
            if (((in_01_08 & 0x20) != 0x20) && ((in_01_08 & 0x40) != 0x40) &&
                ((in_01_08 & 0x80) != 0x80) && ((in_09_16 & 0x01) != 0x01)) {
                system_status = V_STATE_LOCK;
                door_close_done_tick = 0;
            }
        }

        /* Init → Lock */
        if (system_status == V_STATE_INIT) system_status = V_STATE_LOCK;

        /* power_button: Lock ↔ Unlock */
        if (in_09_16 & 0x10) {
            if (lock_press_tick == 0) lock_press_tick = now;
        } else {
            lock_press_tick = 0; lock_released = 1;
        }
        if ((in_09_16 & 0x10) && ((now - lock_press_tick) >= LOCK_PRESS_MS) && lock_released &&
            (lock_release_tick == 0 || (now - lock_release_tick) >= LOCK_IDLE_MS)) {
            if (out_01_08 & 0x80) { CmdExec_Execute(VCMD_LOCK); system_status = V_STATE_LOCK; }
            else { CmdExec_Execute(VCMD_UNLOCK); system_status = V_STATE_IDLE; }
            lock_press_tick = 0; lock_released = 0; lock_release_tick = now;
        }

        /* Idle state logic */
        if (system_status == V_STATE_IDLE) {
            if (!(out_01_08 & 0x80)) system_status = V_STATE_LOCK;
            /* 位置确认 */
            if (!poweron_position_ok) {
                if (in_01_08 & 0x01) { CmdExec_Execute(VCMD_CYLINDER_OPEN); poweron_position_ok = 1; }
                else if (in_01_08 & 0x02) { CmdExec_Execute(VCMD_CYLINDER_CLOSE); poweron_position_ok = 1; system_status = V_STATE_COMPLETE; }
            }
            if (in_01_08 & 0x01) {
                system_status = (out_01_08 & 0x40) ? V_STATE_READY : V_STATE_IDLE;
            } else if (in_01_08 & 0x02) {
                system_status = V_STATE_COMPLETE;
            }
            /* 关门按钮 → Ready */
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02)) {
                if (door_ready_tick == 0) door_ready_tick = now;
            } else { door_ready_tick = 0; }
            if (door_ready_tick && ((now - door_ready_tick) >= DOOR_READY_MS) && (release_start_tick == 0)) {
                CmdExec_Execute(VCMD_DOOR_READY);
                system_status = V_STATE_READY; door_ready_tick = 0;
                poweron_position_ok = 1;
            }
        }

        /* Ready → Running */
        if (system_status == V_STATE_READY) {
            if (!(out_01_08 & 0x40)) system_status = V_STATE_IDLE;
            if ((out_01_08 & 0x02) && !(in_01_08 & 0x02)) system_status = V_STATE_RUNNING;
            /* 双按钮确认关门 */
            uint8_t door_ready_status = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x02) && (release_start_tick == 0)) {
                if (door_close_confirm_tick == 0) door_close_confirm_tick = now;
            } else { door_close_confirm_tick = 0; }
            if (door_close_confirm_tick && ((now - door_close_confirm_tick) >= DOOR_CLOSE_CONFIRM_MS))
                door_ready_status = 1;
            if ((in_09_16 & 0x06) == 0x06 && door_ready_status && (release_start_tick == 0)) {
                if (!(out_01_08 & 0x02)) {
                    CmdExec_Execute(VCMD_CYLINDER_CLOSE);
                    door_close_confirm_tick = 0; release_start_tick = now;
                    system_status = V_STATE_RUNNING;
                    door_close_start_tick = now; door_close_timing = 1;
                    door_close_from_full = (in_01_08 & 0x01) ? 1 : 0;
                    air_last_check_tick = 0;
                }
            }
        }

        /* Running → Complete */
        if (system_status == V_STATE_RUNNING) {
            if ((out_01_08 & 0x02) && (in_01_08 & 0x02)) {
                CmdExec_Execute(VCMD_LED_GREEN);
                system_status = V_STATE_COMPLETE;
                door_close_timing = 0; door_open_confirm_tick = 0;
                if (door_close_from_full) { door_close_default_ms = now - door_close_start_tick; door_close_time_learned = 1; }
                door_close_done_tick = now; door_close_from_full = 0;
            }
        }

        /* Complete → Idle */
        if (system_status == V_STATE_COMPLETE) {
            if ((out_01_08 & 0x01) && (in_01_08 & 0x01)) {
                CmdExec_Execute(VCMD_LED_OFF);
                system_status = V_STATE_IDLE;
                door_open_start_tick = 0;
            }
            /* 开门请求 */
            uint8_t door_complete_status = 0;
            if ((in_09_16 & 0x06) && !(out_01_08 & 0x01) && (in_01_08 & 0x02)) {
                if (door_open_confirm_tick == 0) door_open_confirm_tick = now;
            } else { door_open_confirm_tick = 0; }
            if (door_open_confirm_tick && ((now - door_open_confirm_tick) >= DOOR_OPEN_CONFIRM_MS))
                door_complete_status = 1;
            if ((in_09_16 & 0x06) && door_complete_status && (release_start_tick == 0)) {
                if (!(out_01_08 & 0x01)) {
                    CmdExec_Execute(VCMD_CYLINDER_OPEN);
                    door_open_start_tick = now; door_open_confirm_tick = 0;
                    release_start_tick = now;
                }
            }
        }

        /* 气压检测 */
        if ((in_01_08 & 0x02) && door_close_done_tick) {
            uint32_t elapsed = now - door_close_done_tick;
            if (elapsed >= 3000) {
                if (!air_last_check_tick || ((now - air_last_check_tick) >= 1500)) {
                    if ((in_01_08 & 0x20) != 0x20) {
                        /* 气压低告警 */;
                    }
                    air_last_check_tick = now;
                }
            }
        }
    }

    /* ── 5. 按钮释放检测 ── */
    if (!(in_09_16 & 0x06)) {
        if (release_start_tick && ((now - release_start_tick) >= RELEASE_DELAY_MS))
            release_start_tick = 0;
    } else {
        if (release_start_tick) release_start_tick = now;
    }

    RamVector_SetState((Vector_SysState_t)system_status);
    RamVector_Heartbeat();
    osDelay(50);
  }
  /* USER CODE END StateVectorTask */
}

/* SCPITimerCallback function */
void SCPITimerCallback(void *argument)
{
  /* USER CODE BEGIN SCPITimerCallback */

  /* USER CODE END SCPITimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

