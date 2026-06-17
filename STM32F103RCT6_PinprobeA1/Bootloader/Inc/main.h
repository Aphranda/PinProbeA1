/*
 * Minimal bootloader main header.
 */

#ifndef BOOTLOADER_INC_MAIN_H_
#define BOOTLOADER_INC_MAIN_H_

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI3_CS_Pin GPIO_PIN_13
#define SPI3_CS_GPIO_Port GPIOC

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_INC_MAIN_H_ */
