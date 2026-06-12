/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "cmsis_os.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "StateMachine.h"
#include "BsmRelay.h"
#include "RS485.h"
#include "scpi/scpi.h"
#include "scpi-def.h"
#include "flash.h"
#include "ram_vector.h"
#include "state_vector.h"
#include "cmd_exec.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart3_tx;

extern uint8_t* Uart1_BuffIsReady;
extern uint8_t* Uart1_BuffOccupied;

extern uint8_t* Uart3_BuffIsReady;
extern uint8_t* Uart3_BuffOccupied;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief 上电自检 (Power-On Self Test)
  * @note  检查 RCC 时钟、Flash 配置 CRC、RS485 从机响应。
  *        使用阻塞 HAL_UART_Transmit 确保输出不丢失。
  *        RS485 测试期间喂狗防止复位。
  * @retval 0=全部通过, 非0=故障码 (bit0=RCC, bit1=Flash, bit2=RS485)
  */
static uint8_t PowerOnSelfTest(void)
{
    uint8_t fault = 0;
    char buf[64];
    int len;

    /* 辅助宏: 阻塞发送, 确保 POST 日志不丢 */
    #define POST_PRINT(s)  HAL_UART_Transmit(&huart1, (uint8_t*)(s), strlen(s), 100)

    POST_PRINT("[POST] === Self Test ===\r\n");

    /* 1. RCC 时钟 */
    {
        uint32_t sysclk = HAL_RCC_GetSysClockFreq();
        if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)  fault |= 0x01;
        if (sysclk < 71000000UL || sysclk > 73000000UL)    fault |= 0x01;
        len = snprintf(buf, sizeof(buf), "[POST] RCC   %s (SYSCLK=%luHz)\r\n",
                       (fault & 0x01) ? "FAIL" : "OK", sysclk);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100);
    }

    /* 2. Flash 配置 CRC */
    {
        if (!Flash_IsValid()) {
            fault |= 0x02;
            POST_PRINT("[POST] Flash FAIL (using defaults)\r\n");
        } else {
            len = snprintf(buf, sizeof(buf), "[POST] Flash OK (v=%08lX)\r\n",
                           (unsigned int)Flash_GetConfig()->version);
            HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100);
        }
    }

    /* 3. RS485 从机响应 (期间喂狗, IO_Read 单次约 60ms) */
    {
        HAL_IWDG_Refresh(&hiwdg);
        uint8_t io_buf[2] = {0};
        bool ok = IO_Read(3, 2, io_buf);
        HAL_IWDG_Refresh(&hiwdg);
        if (!ok) fault |= 0x04;
        len = snprintf(buf, sizeof(buf), "[POST] RS485 %s (IN=0x%02X,0x%02X)\r\n",
                       ok ? "OK" : "FAIL", io_buf[0], io_buf[1]);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100);
    }

    len = snprintf(buf, sizeof(buf), "[POST] === %s (code=0x%02X) ===\r\n",
                   fault ? "FAILED" : "PASSED", fault);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100);

    #undef POST_PRINT
    return fault;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_IWDG_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_SPI3_Init();
  MX_CAN_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

  __HAL_DMA_ENABLE_IT(&hdma_usart1_tx, DMA_IT_TC);
  __HAL_DMA_ENABLE_IT(&hdma_usart3_tx, DMA_IT_TC);

  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
  __HAL_UART_CLEAR_IDLEFLAG(&huart3);

  HAL_UART_Receive_DMA(&huart1, Uart1_BuffOccupied, MAX_RX_LEN);
  HAL_UART_Receive_DMA(&huart3, Uart3_BuffOccupied, MAX_RX_LEN);

  /* 初始化 Flash 配置 (从 Flash 加载或使用默认值) */
  Flash_Init();

  /* 初始化 SCPI 上下文，使用运行时 IDN 缓冲区 (而非宏常量) */
  SCPI_Init(&scpi_context,
    scpi_commands,
    &scpi_interface,
    scpi_units_def,
    scpi_idn_buf1, scpi_idn_buf2, scpi_idn_buf3, scpi_idn_buf4,
    scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
    scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

  /* 从 Flash 同步 IDN 字符串到运行时缓冲区 (覆盖默认值) */
  SCPI_SyncIdnFromFlash();

  /* 上电自检 */
  (void)PowerOnSelfTest();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief SCPI命令处理任务
  * 
  * 该任务循环检测串口1接收缓冲区(Uart1_BuffIsReady)是否有SCPI命令数据，
  * 如果有，则调用SCPI_Input进行命令解析和处理，处理完成后清空缓冲区。
  * 每次循环结束后延时10ms。
  */
void SCPITask(void *argument)
{
  /* USER CODE BEGIN SCPITask */
  /* 无限循环 */
  for(;;)
  {
    if (Uart1_RxLength > 3)  // 至少4字节（如 *IDN? + 换行）
    {
      Uart1_BuffIsReady[Uart1_RxLength] = '\0';  // 确保零结尾
      SCPI_Input(&scpi_context, (const char *)Uart1_BuffIsReady, Uart1_RxLength);
      Uart1_RxLength = 0;  // 消费完毕
    }
    osDelay(10);                   // 任务延时10ms
  }
  /* USER CODE END SCPITask */
}

/**
  * @brief ModBus及主状态机处理任务
  * 
  * 该任务循环调用StateMachine_Input()，用于处理系统主状态机逻辑（如门控、继电器等）。
  * RS485 IO 读取 + 向量表命令执行, SysTimer 控制周期。
  */
void ModBusTask(void *argument)
{
  /* USER CODE BEGIN ModBusTask */
  (void)argument;
  osDelay(1000); // 延迟于IO拓展板启动

  for(;;)
  {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

    uint8_t in_buf[2] = {0}, out_buf[2] = {0};
    bool ok = IO_Read(5, 2, in_buf);
    ok = IO_Read(5, 1, out_buf) && ok;
    SetRS485_Ok(ok);

    /* 更新向量表 IO 镜像 */
    Vector_IOState_t io;
    const Vector_IOState_t* old_io = RamVector_GetLocalIO();
    memcpy(&io, old_io, sizeof(io));
    io.raw_in_lo  = in_buf[0];
    io.raw_in_hi  = in_buf[1];
    io.raw_out_lo = out_buf[0];
    io.raw_out_hi = out_buf[1];
    io.rs485_ok   = ok ? 1 : 0;
    RamVector_UpdateLocalIO(&io);

    /* 执行待处理命令 */
    Vector_Cmd_t cmd = RamVector_GetCmd();
    if (cmd != VCMD_NONE) {
        CmdExec_Execute(cmd);
        RamVector_ClearCmd();
    }

    /* 周期由 SysTimer 控制 */
  }
  /* USER CODE END ModBusTask */
}

/* StateVectorTask — 覆盖 freertos.c 中的 __weak 空壳 */
void StateVectorTask(void *argument)
{
  (void)argument;
  RamVector_Init(0);  /* 单机出厂模式 */
  osDelay(500); // 等 ModBusTask 就绪
  for(;;) {
    osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    StateVector_Input();
  }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM1)
  {
    tim1_ms++;
  }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
