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
#include "can.h"
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
  .priority = (osPriority_t) osPriorityLow,
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
  .priority = (osPriority_t) osPriorityNormal2,
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
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for CANRx */
osThreadId_t CANRxHandle;
const osThreadAttr_t CANRx_attributes = {
  .name = "CANRx",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for CANTx */
osThreadId_t CANTxHandle;
const osThreadAttr_t CANTx_attributes = {
  .name = "CANTx",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANProtocol */
osThreadId_t CANProtocolHandle;
const osThreadAttr_t CANProtocol_attributes = {
  .name = "CANProtocol",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for Snapshot */
osThreadId_t SnapshotHandle;
const osThreadAttr_t Snapshot_attributes = {
  .name = "Snapshot",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CanRxQueue */
osMessageQueueId_t CanRxQueueHandle;
const osMessageQueueAttr_t CanRxQueue_attributes = {
  .name = "CanRxQueue"
};
/* Definitions for CanTxQueue */
osMessageQueueId_t CanTxQueueHandle;
const osMessageQueueAttr_t CanTxQueue_attributes = {
  .name = "CanTxQueue"
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
void CANRxTask(void *argument);
void CANTxTask(void *argument);
void CANProtocolTask(void *argument);
void SnapshotTask(void *argument);
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

  /* Create the queue(s) */
  /* creation of CanRxQueue */
  CanRxQueueHandle = osMessageQueueNew (16, sizeof(CanFrame_t), &CanRxQueue_attributes);

  /* creation of CanTxQueue */
  CanTxQueueHandle = osMessageQueueNew (16, sizeof(CanFrame_t), &CanTxQueue_attributes);

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

  /* creation of CANRx */
  CANRxHandle = osThreadNew(CANRxTask, NULL, &CANRx_attributes);

  /* creation of CANTx */
  CANTxHandle = osThreadNew(CANTxTask, NULL, &CANTx_attributes);

  /* creation of CANProtocol */
  CANProtocolHandle = osThreadNew(CANProtocolTask, NULL, &CANProtocol_attributes);

  /* creation of Snapshot */
  SnapshotHandle = osThreadNew(SnapshotTask, NULL, &Snapshot_attributes);

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

/* USER CODE BEGIN Header_CANRxTask */
/**
* @brief Function implementing the CANRx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CANRxTask */
__weak void CANRxTask(void *argument)
{
  /* USER CODE BEGIN CANRxTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CANRxTask */
}

/* USER CODE BEGIN Header_CANTxTask */
/**
* @brief Function implementing the CANTx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CANTxTask */
__weak void CANTxTask(void *argument)
{
  /* USER CODE BEGIN CANTxTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CANTxTask */
}

/* USER CODE BEGIN Header_CANProtocolTask */
/**
* @brief Function implementing the CANProtocol thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CANProtocolTask */
__weak void CANProtocolTask(void *argument)
{
  /* USER CODE BEGIN CANProtocolTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CANProtocolTask */
}

/* USER CODE BEGIN Header_SnapshotTask */
/**
* @brief Function implementing the Snapshot thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SnapshotTask */
__weak void SnapshotTask(void *argument)
{
  /* USER CODE BEGIN SnapshotTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END SnapshotTask */
}

/* SysTimerCallback function */
__weak void SysTimerCallback(void *argument)
{
  /* USER CODE BEGIN SysTimerCallback */
  osThreadFlagsSet(ModBusHandle, 0x01);
  osThreadFlagsSet(StateVectorHandle, 0x01);
  /* USER CODE END SysTimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

