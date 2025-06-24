/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.h
  * @brief   This file contains the headers of the interrupt handlers.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_IT_H
#define __STM32F1xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include <stdarg.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define MAX_RX_LEN (256U)
#define MAX_TX_LEN (512U)
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t* usart1_buff_IsReady;
extern uint8_t* usart1_buff_Occupied;

extern uint8_t* usart3_buff_IsReady;
extern uint8_t* usart3_buff_Occupied;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void DMA1_Channel2_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);
void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void TIM5_IRQHandler(void);
/* USER CODE BEGIN EFP */
void DMA_Usart1_Tx_Data(uint8_t *buffer, uint16_t size); // DMA Usart1 Tx Data
void DMA_Usart3_Tx_Data(uint8_t *buffer, uint16_t size); // DMA Usart3 Tx Data

void U1_Printf(char *format, ...); // Usart1 print
void U3_Printf(char *format, ...); // Usart3 print

void Usart1_Tx_Wait(void); // Transmit Delay
void Usart3_Tx_Wait(void); // Transmit Delay
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_IT_H */
