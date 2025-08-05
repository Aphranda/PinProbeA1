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

uint8_t* usart1_buff_IsReady = Usart1_RX_BUF2;  // The pointer points to BUF2
uint8_t* usart1_buff_Occupied = Usart1_RX_BUF1; // The pointer points to BUF1

volatile uint8_t Usart3_TX_Flag = 0;
uint8_t Usart3_TX_BUF[MAX_TX_LEN];
uint8_t Usart3_RX_BUF1[MAX_RX_LEN];
uint8_t Usart3_RX_BUF2[MAX_RX_LEN];

uint8_t Usart3_RX_BUF1_IsReady = 0;

uint8_t* usart3_buff_IsReady = Usart3_RX_BUF2;
uint8_t* usart3_buff_Occupied = Usart3_RX_BUF2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
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
      usart1_buff_IsReady = Usart1_RX_BUF2;
      usart1_buff_Occupied = Usart1_RX_BUF1;
      Usart1_RX_BUF1_IsReady = 0;
    }
    else
    {
      usart1_buff_IsReady = Usart1_RX_BUF1;
      usart1_buff_Occupied = Usart1_RX_BUF2;
      Usart1_RX_BUF1_IsReady = 1;
    }
    memset((uint8_t *)usart1_buff_Occupied, 0, MAX_RX_LEN);	 // clear receive cache
  } 
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
  // SCPI_Input(&scpi_context, (const char *)usart1_buff_IsReady, data_length);
  HAL_UART_Receive_DMA(&huart1, usart1_buff_Occupied, MAX_RX_LEN); // restart receive DMA
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
  

  // double cache
    if(Usart3_RX_BUF1_IsReady)
    {
      usart3_buff_IsReady = Usart3_RX_BUF2;
      usart3_buff_Occupied = Usart3_RX_BUF1;
      Usart3_RX_BUF1_IsReady = 0;
    }
    else
    {
      usart3_buff_IsReady = Usart3_RX_BUF1;
      usart3_buff_Occupied = Usart3_RX_BUF2;
      Usart3_RX_BUF1_IsReady = 1;
    }
    memset((uint8_t *)usart3_buff_Occupied, 0, MAX_RX_LEN);	 // clear receive cache
  }
  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */
  HAL_UART_Receive_DMA(&huart3, usart3_buff_Occupied, MAX_RX_LEN); // restart receive DMA
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

/* USER CODE BEGIN 1 */
void DMA_Usart1_Tx_Data(uint8_t *buffer, uint16_t size) // DMA Usart1 Tx Data
{
  Usart1_Tx_Wait();
  Usart1_TX_Flag = 1;
  HAL_UART_Transmit_DMA(&huart1, buffer, size);
}

void DMA_Usart3_Tx_Data(uint8_t *buffer, uint16_t size) // DMA Usart3 Tx Data
{
  Usart3_Tx_Wait();
  Usart3_TX_Flag = 1;
  HAL_UART_Transmit_DMA(&huart3, buffer, size);
}

void Usart1_Tx_Wait(void) // Transmit Delay
{
  uint16_t delay = 20000;
  while(Usart1_TX_Flag)
  {
    delay--;
    if(delay == 0)
      return;
  }
}

void Usart3_Tx_Wait(void) // Transmit Delay
{
  uint16_t delay = 20000;
  while(Usart3_TX_Flag)
  {
    delay--;
    if(delay == 0)
      return;
  }
}

void U1_Printf(char *format, ...) // Usart1 print
{
  va_list arg_ptr;

  Usart1_Tx_Wait(); 

  va_start(arg_ptr, format);

  vsnprintf((char *)Usart1_TX_BUF, MAX_TX_LEN + 1, format, arg_ptr);

  va_end(arg_ptr);

  DMA_Usart1_Tx_Data(Usart1_TX_BUF, strlen((const char *)Usart1_TX_BUF)); 
}

void U3_Printf(char *format, ...) // Usart3 print
{
  va_list arg_ptr;

  Usart3_Tx_Wait(); 

  va_start(arg_ptr, format);

  vsnprintf((char *)Usart3_TX_BUF, MAX_TX_LEN + 1, format, arg_ptr);

  va_end(arg_ptr);
 
  DMA_Usart3_Tx_Data(Usart3_TX_BUF, strlen((const char *)Usart3_TX_BUF)); 
}
/* USER CODE END 1 */
