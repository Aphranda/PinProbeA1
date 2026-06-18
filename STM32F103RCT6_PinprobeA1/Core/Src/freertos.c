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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MODBUS_POLL_DIV 2U
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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SysTimer */
osTimerId_t SysTimerHandle;
const osTimerAttr_t SysTimer_attributes = {
  .name = "SysTimer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void SCPITask(void *argument);
void ModBusTask(void *argument);
void WatchDogTask(void *argument);
void StateVectorTask(void *argument);
void SysTimerCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of SysTimer */
  SysTimerHandle = osTimerNew(SysTimerCallback, osTimerPeriodic, NULL, &SysTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  osTimerStart(SysTimerHandle, 25);  /* 25ms 系统节拍 */
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
  /* 集群健康监控 — 单机为占位, 多机时扫描 node_status[1..7] */
  osDelay(2000);
  for(;;)
  {
    // TODO: 多机模式时检查 ram_vector.node_status[i].heartbeat/online
    osDelay(1000); /* 1s 周期 */
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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StateVectorTask */
}

/* SysTimerCallback function */
__weak void SysTimerCallback(void *argument)
{
  /* USER CODE BEGIN SysTimerCallback */
  static uint8_t modbus_div = 0U;
  (void)argument;

  osThreadFlagsSet(StateVectorHandle, 0x01);  /* 25ms state machine */

  if (++modbus_div >= MODBUS_POLL_DIV) {
    modbus_div = 0U;
    osThreadFlagsSet(ModBusHandle, 0x01);     /* 50ms RS485 polling */
  }
  /* USER CODE END SysTimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

