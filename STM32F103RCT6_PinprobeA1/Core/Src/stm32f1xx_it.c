/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "scpi/scpi.h"
#include "scpi-def.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

volatile uint8_t Usart1_TX_Flag = 0;  // uart1 transmit flag, beginning transmit flag equal 1
uint8_t Usart1_TX_BUF[MAX_TX_LEN];    // uart1 printf TX buff
uint8_t Usart1_RX_BUF1[MAX_RX_LEN];   // uart1 DMA RX buff1
uint8_t Usart1_RX_BUF2[MAX_RX_LEN];   // uart1 DMA RX buff2

uint8_t Usart1_RX_BUF1_IsReady = 0;   // double buffer flag, which Cache occupied
// 0: Usart1_RX_BUF1 Occupied, Usart1_RX_BUF2 Ready
// 1: Usart1_RX_BUF1 Occupied, Usart1_RX_BUF2 Ready

uint8_t* Uart1_BuffIsReady = Usart1_RX_BUF2;  // The pointer points to BUF2
uint8_t* Uart1_BuffOccupied = Usart1_RX_BUF1; // The pointer points to BUF1

volatile uint8_t Usart3_TX_Flag = 0;
uint8_t Usart3_TX_BUF[MAX_TX_LEN];
uint8_t Usart3_RX_BUF1[MAX_RX_LEN];
uint8_t Usart3_RX_BUF2[MAX_RX_LEN];

uint8_t Usart3_RX_BUF1_IsReady = 0;
uint32_t Uart3_RxLength = 0;           // 最近一次USART3接收帧长度

uint8_t* Uart3_BuffIsReady = Usart3_RX_BUF2;
uint8_t* Uart3_BuffOccupied = Usart3_RX_BUF2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim5;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel2 global interrupt.
  */
void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */
  if(__HAL_DMA_GET_FLAG(&hdma_usart3_tx, DMA_FLAG_TC2) != RESET)
  {
    __HAL_UART_CLEAR_IDLEFLAG(&huart3); // clear uart idle flag
    Usart3_TX_Flag = 0;                 // reset tx flag
    huart3.gState = HAL_UART_STATE_READY;
    hdma_usart3_tx.State = HAL_DMA_STATE_READY;
  }
  /* USER CODE END DMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */

  /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel3 global interrupt.
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

  /* USER CODE END DMA1_Channel3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_rx);
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */
  if(__HAL_DMA_GET_FLAG(&hdma_usart1_tx, DMA_FLAG_TC4) != RESET)
  {
    __HAL_UART_CLEAR_IDLEFLAG(&huart1); // clear uart idle flag

    Usart1_TX_Flag = 0; // reset tx flag

    huart1.gState = HAL_UART_STATE_READY;
    hdma_usart1_tx.State = HAL_DMA_STATE_READY;
  }
  /* USER CODE END DMA1_Channel4_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */

  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */
  if(__HAL_DMA_GET_FLAG(&hdma_usart3_tx, DMA_FLAG_TC5) != RESET)
  {
    __HAL_UART_CLEAR_IDLEFLAG(&huart3); // clear uart idle flag

    Usart3_TX_Flag = 0; // reset tx flag

    huart3.gState = HAL_UART_STATE_READY;
    hdma_usart3_tx.State = HAL_DMA_STATE_READY;
  }
  /* USER CODE END DMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */

  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles USB high priority or CAN TX interrupts.
  */
void USB_HP_CAN1_TX_IRQHandler(void)
{
  /* USER CODE BEGIN USB_HP_CAN1_TX_IRQn 0 */

  /* USER CODE END USB_HP_CAN1_TX_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN USB_HP_CAN1_TX_IRQn 1 */

  /* USER CODE END USB_HP_CAN1_TX_IRQn 1 */
}

/**
  * @brief This function handles USB low priority or CAN RX0 interrupts.
  */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 0 */

  /* USER CODE END USB_LP_CAN1_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 1 */

  /* USER CODE END USB_LP_CAN1_RX0_IRQn 1 */
}

/**
  * @brief This function handles CAN RX1 interrupt.
  */
void CAN1_RX1_IRQHandler(void)
{
  /* USER CODE BEGIN CAN1_RX1_IRQn 0 */

  /* USER CODE END CAN1_RX1_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN CAN1_RX1_IRQn 1 */

  /* USER CODE END CAN1_RX1_IRQn 1 */
}

/**
  * @brief This function handles CAN SCE interrupt.
  */
void CAN1_SCE_IRQHandler(void)
{
  /* USER CODE BEGIN CAN1_SCE_IRQn 0 */

  /* USER CODE END CAN1_SCE_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN CAN1_SCE_IRQn 1 */

  /* USER CODE END CAN1_SCE_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */

  /* USER CODE END TIM1_UP_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
  if (RESET != __HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))
  {
    HAL_UART_DMAStop(&huart1); // stop DMA


    // double cache
    if(Usart1_RX_BUF1_IsReady)
    {
      Uart1_BuffIsReady = Usart1_RX_BUF2;
      Uart1_BuffOccupied = Usart1_RX_BUF1;
      Usart1_RX_BUF1_IsReady = 0;
    }
    else
    {
      Uart1_BuffIsReady = Usart1_RX_BUF1;
      Uart1_BuffOccupied = Usart1_RX_BUF2;
      Usart1_RX_BUF1_IsReady = 1;
    }
    memset((uint8_t *)Uart1_BuffOccupied, 0, MAX_RX_LEN);	 // clear receive cache
  } 
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
  // SCPI_Input(&scpi_context, (const char *)Uart1_BuffIsReady, data_length);
  HAL_UART_Receive_DMA(&huart1, Uart1_BuffOccupied, MAX_RX_LEN); // restart receive DMA
  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */
  if (RESET != __HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
  {
    HAL_UART_DMAStop(&huart3); // stop DMA

    uint32_t data_length = MAX_RX_LEN - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
    Uart3_RxLength = data_length; // 记录接收长度供上层校验

  // double cache
    if(Usart3_RX_BUF1_IsReady)
    {
      Uart3_BuffIsReady = Usart3_RX_BUF2;
      Uart3_BuffOccupied = Usart3_RX_BUF1;
      Usart3_RX_BUF1_IsReady = 0;
    }
    else
    {
      Uart3_BuffIsReady = Usart3_RX_BUF1;
      Uart3_BuffOccupied = Usart3_RX_BUF2;
      Usart3_RX_BUF1_IsReady = 1;
    }
    memset((uint8_t *)Uart3_BuffOccupied, 0, MAX_RX_LEN);	 // clear receive cache
  }
  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */
  HAL_UART_Receive_DMA(&huart3, Uart3_BuffOccupied, MAX_RX_LEN); // restart receive DMA
  /* USER CODE END USART3_IRQn 1 */
}

/**
  * @brief This function handles TIM5 global interrupt.
  */
void TIM5_IRQHandler(void)
{
  /* USER CODE BEGIN TIM5_IRQn 0 */

  /* USER CODE END TIM5_IRQn 0 */
  HAL_TIM_IRQHandler(&htim5);
  /* USER CODE BEGIN TIM5_IRQn 1 */

  /* USER CODE END TIM5_IRQn 1 */
}

/**
  * @brief This function handles SPI3 global interrupt.
  */
void SPI3_IRQHandler(void)
{
  /* USER CODE BEGIN SPI3_IRQn 0 */

  /* USER CODE END SPI3_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi3);
  /* USER CODE BEGIN SPI3_IRQn 1 */

  /* USER CODE END SPI3_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel1 global interrupt.
  */
void DMA2_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel1_IRQn 0 */

  /* USER CODE END DMA2_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi3_rx);
  /* USER CODE BEGIN DMA2_Channel1_IRQn 1 */

  /* USER CODE END DMA2_Channel1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel2 global interrupt.
  */
void DMA2_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel2_IRQn 0 */

  /* USER CODE END DMA2_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi3_tx);
  /* USER CODE BEGIN DMA2_Channel2_IRQn 1 */

  /* USER CODE END DMA2_Channel2_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void Uart1_Printf(char *format, ...) // Usart1 print (非阻塞, TX忙时跳过)
{
  if (Usart1_TX_Flag) return;

  va_list arg_ptr;
  va_start(arg_ptr, format);
  vsnprintf((char *)Usart1_TX_BUF, MAX_TX_LEN, format, arg_ptr);
  va_end(arg_ptr);

  Usart1_TX_Flag = 1;
  HAL_UART_Transmit_DMA(&huart1, Usart1_TX_BUF, strlen((const char *)Usart1_TX_BUF));
}

void Uart3_Printf(char *format, ...) // Usart3 print (非阻塞, TX忙时跳过)
{
  if (Usart3_TX_Flag) return;

  va_list arg_ptr;
  va_start(arg_ptr, format);
  vsnprintf((char *)Usart3_TX_BUF, MAX_TX_LEN, format, arg_ptr);
  va_end(arg_ptr);

  Usart3_TX_Flag = 1;
  HAL_UART_Transmit_DMA(&huart3, Usart3_TX_BUF, strlen((const char *)Usart3_TX_BUF));
}
/* USER CODE END 1 */
